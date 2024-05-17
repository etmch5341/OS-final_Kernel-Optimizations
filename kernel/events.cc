#include "events.h"

PerCPU<CFSScheduler> schedulers{};

namespace impl {
Queue<Event, SpinLock> ready_queue{};

struct PQEntry {
    uint32_t const at;
    Event* e;
    PQEntry* next = nullptr;
    PQEntry(uint32_t const at, Event* e) : at(at), e(e) {}
};

struct PQ {
    PQEntry* head = nullptr;
    SpinLock lock{};  // is this wise? no because add is O(n)

    void add(uint32_t const at, Event* e) {
        auto pqe = new PQEntry(at, e);
        lock.lock();
        auto p = head;
        auto pprev = &head;
        while (p != nullptr) {
            if (p->at > at) break;
            pprev = &p->next;
            p = p->next;
        }
        pqe->next = p;
        *pprev = pqe;
        lock.unlock();
    }

    Event* remove_if_ready() {
        lock.lock();
        auto pqe = head;
        if (pqe == nullptr) {
            lock.unlock();
            return nullptr;
        }
        if (pqe->at <= Pit::jiffies) {
            auto e = pqe->e;
            head = pqe->next;
            lock.unlock();
            delete pqe;
            return e;
        }
        lock.unlock();
        return nullptr;
    }
} pq;

void timed(const uint32_t at, Event* e) {
    pq.add(at, e);
}

PerCPU<Event*> pending_event{};

void manage_pending() {
    if (pending_event.mine() != nullptr) {
        Event* e = pending_event.mine();
        pending_event.mine() = nullptr;
        //old current task has finished so we will replace it
        schedulers.mine().getNewCurrent();
        // Debug::printf("Task completed on core %d\n", SMP::me());
        delete e;
    }
}

}  // namespace impl

void event_loop() {
    using namespace impl;

    manage_pending();

    // Debug::printf("| core#%d entring event_loop\n", SMP::me());
    while (true) {
        while (true) {
            auto e = pq.remove_if_ready();
            if (e == nullptr) break;
            schedulers.mine().insert(e);
            //ready_queue.add(e);
        }
        
        //schedulers.mine().insert(ready_queue.remove());
        auto e = schedulers.mine().currTask;
        //auto e = ready_queue.remove();
        if (e == nullptr) {
            for(uint32_t i = 1; i < kConfig.totalProcs; i++){
                //work stealing
                e = schedulers.forCPU((SMP::me() + i) % 4).takeNext();
                //spaghetti time
                if(e != nullptr){
                    //Debug::printf("Core %d has stolen work from core %d\n", SMP::me(), ((SMP::me() + i) % 4));
                    schedulers.mine().insert(e);
                    pending_event.mine() = e;
                    e->doit();
                    manage_pending();
                }
            }
            //pause();
        }
        else {
            //impl::schedulers.mine().currTask = impl::schedulers.mine().takeNext();
            pending_event.mine() = e;
            e->doit();
            manage_pending();
        }
    }
}
