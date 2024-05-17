#pragma once

#include "stdint.h"
#include "queue.h"
#include "pit.h"
#include "shared.h"
#include "genericds.h"

#include <coroutine>

// Implementation details, we use a namespace to protect against
// accidental direct use in test cases
namespace impl {

    // implementation hints, feel free to use, remove, replace, enhance, ...

    struct Event {
        Event* next = nullptr;
        virtual void doit() = 0;
        int vRunTime = 0;
        virtual ~Event() {}
    };

    template <typename Work>
    struct EventWithWork: public Event {
        const Work work;
        explicit inline EventWithWork(const Work& work): work(work) {}
        void doit() override {
            work();
        }
    };

    extern Queue<Event, SpinLock> ready_queue;

    template <typename Work>
    void run_at(const uint32_t at, const Work& work) {
        if (at > Pit::jiffies) {
            auto e = new EventWithWork([at, work] {
                run_at(at, work);
            });
            ready_queue.add(e);
        } else {
            auto e = new EventWithWork(work);
            ready_queue.add(e);
        } 
    }

    extern void timed(const uint32_t at, Event* e);
}

template <>
struct Generic::CompareFunction<impl::Event*, impl::Event*> {
    int32_t operator()(impl::Event* a, impl::Event* other) {
        if(a == nullptr){
            return -1;
        }
        if(other == nullptr){
            return 1;
        }

        if (a->vRunTime < other->vRunTime){
            return -1;
        }
        if (a->vRunTime > other->vRunTime) {
            return 1;
        }
        return 0;
    }
};

struct CFSScheduler{
    Generic::RBTree<impl::Event*, SpinLock>* schedTree;
    impl::Event* next = nullptr;
    impl::Event* currTask = nullptr;
    SpinLock* splock;
    int id;

    CFSScheduler(){
        schedTree = new Generic::RBTree<impl::Event*, SpinLock>(nullptr);
        next = nullptr;
        currTask = nullptr;
        splock = new SpinLock();
        // id = -1; //for debugging work stealing
    }

    //CompareFunction<Event*, Event*> comparator;

    // CFSScheduler(Event* next, Event* currTask) : schedTree(),
    //                                              next(next),
    //                                              currTask(currTask) {}
                                                    
    void insert(impl::Event* ev){
        splock->lock();
        // this->id = SMP::me();
        // Debug::printf("Inserting into tree for core %d\n", SMP::me());

        if(ev == nullptr){
            splock->unlock();
            return;
        }

        if(this->currTask == nullptr){
            this->currTask = ev;
            splock->unlock();
            return;
        }
        
        if((this->next == nullptr) || (ev->vRunTime < this->next->vRunTime)){
            // Debug::printf("Insert: updated next node for core %d\n", SMP::me());
            this->next = ev;
        }
        schedTree->insert(ev);
        // Debug::printf("Insert into rbtree complete for core %d\n", SMP::me());
        splock->unlock();
    }

    //Used in case where current task has finished
    impl::Event* getNewCurrent(){
        splock->lock();
        // Debug::printf("Getting new task for core %d\n", SMP::me());
        if(this->next == nullptr){
            this->currTask = nullptr;
            splock->unlock();
            return nullptr;
        }

        this->currTask = this->next;
        this->next = schedTree->search_just_less(this->currTask);
        schedTree->remove(this->currTask);
        splock->unlock();
        return this->currTask;
    }

    //Used in case where current task didn't finish
    impl::Event* replaceCurrent(){
        splock->lock();
        this->currTask->vRunTime = this->currTask->vRunTime + 1; // preempts at milisecond
        // Debug::printf("Replacing task for core %d\n", SMP::me());
        if((this->next == nullptr) || (this->currTask->vRunTime <= this->next->vRunTime)){
            // Debug::printf("in if replace\n");
            splock->unlock();
            return this->currTask;
        }

        schedTree->insert(this->currTask);
        this->currTask = this->next;
        this->next = schedTree->search_just_less(this->currTask);
        splock->unlock();
        return this->currTask;
    }


    //used in work stealing
    impl::Event* takeNext(){
        splock->lock();
        Debug::printf("Steal: Scheduler id: %d\n", this->id);
        Debug::printf("Steal: Core id: %d\n", SMP::me());
        if(this->next == nullptr){
            splock->unlock();
            return nullptr;
        }
        
        impl::Event* output = this->next;
        schedTree->remove(this->next);
        this->next = schedTree->search_just_less(output);
        // Debug::printf("Core %d has stolen work from core %d\n", SMP::me(), this->id);
        splock->unlock();
        return output;
    }

};

extern PerCPU<CFSScheduler> schedulers;


/******************/
/* The public API */
/******************/

// Schedules some conccurent work in the future.
// No sooner than "ms" milli-seoncs
template <typename Work>
inline void go(const Work& work, uint32_t const delay=0) {
    auto e = new impl::EventWithWork(work);
    //e->vRuntime = Pit::jiffies;
    // Debug::printf("In go call\n");
    if(delay == 0){

        // if(schedulers.mine().currTask == nullptr){
        //     // Debug::printf("Go curr task update for core %d\n", SMP::me());
        //     // schedulers.mine().id = SMP::me();
        //     schedulers.mine().currTask = e;
        // }
        // else{
        //     // Debug::printf("Go insert for core %d\n", SMP::me());
        //     schedulers.mine().insert(e);
        // }

        schedulers.mine().insert(e);

        
    }
    else{
        timed(e->vRunTime + delay + 1, e);
    }

    // if (delay == 0) {
    //     impl::ready_queue.add(e);
    // } else {
    //     timed(Pit::jiffies+delay+1, e);
    // }
}

// Called in "init.cc" when a core is idle. Beware of stack overflow
extern void event_loop();

class co_delay {
    uint32_t const ms;
public:
    explicit co_delay(uint32_t ms): ms(ms) {}

    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<void> handle) noexcept {
        go(handle,ms);
    }

    void await_resume() noexcept {
    }
};
