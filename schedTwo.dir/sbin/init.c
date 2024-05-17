#include "libc.h"


/*
A problem that can arise with blocking APIT calls inside of the kernel is that if you have processes that spend a lot of time making sys calls, then
those processes hog the cores and starve other processes. What this test case does is create a lot of children processes which are in infinite loops
until the parent releases them. These processes will hog the cores while making sys calls, and you will need to limit their run time by forcing them 
to the back of the event queue  them in order to give the parent a chance to run frequently enough that it will be able to unblock the children.

I would reccomend making a flag in each core that tracks if that core has been interrupted by the APIT handler while performing a sys call, setting it to true and 
then returning back to the sys call. Then, at the end of the sys call, check if the flag has been set and if it has, preempt the process. This will force the greedy 
children back to the end of the event queue and make it more likely that the parent will be picked up by a core.
*/

int forkCounter = 0;
int volatile* shared = (int volatile*) 0xF0000000;

int main(int argc, char** argv) {
    puts("*** Starting test!");

    for(int i = 0; i < 5; i++){
        if(fork() < 1){
            forkCounter++;
        }
    }

    if(forkCounter == 5){
        for(volatile int i = 0; i < (1 << 22); i++){
            //doing nothing, the int being volitile stops 
            //the compiler from optimizing out this loop
        }
        *shared = 1;
        for(int i = 0; i < 32; i++){
            join();
        }
        puts("\n*** Children have finished merging");
        puts("*** Test passed!");
    }
    else{
        while(*shared != 1){
            write(1, ".", 1);
        }
        exit(0);
    }
    
    shutdown();
    return 0;
}