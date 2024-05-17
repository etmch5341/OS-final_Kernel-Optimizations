#include "libc.h"

extern unsigned _end;

volatile int progressCounter = 0;

int LIMIT = 3000;

int main(int argc, char** argv) {
    printf("*** Test case started!\n");
    int id = fork();

    while(progressCounter != LIMIT){
        printf("Progress counter: %d\n", progressCounter);
        if(id == 1){
            for(volatile unsigned int i = 0; i <= 0; i++){
                //infinite loop, wasting time
            }
        }
        else{
            progressCounter = progressCounter + 1;

            if(fork() != 0){
                exit(0);
            }

        }
    }

    printf("Done with loop\n");

    if(progressCounter == LIMIT){
        printf("*** Test passed!\n");
    }
    else{
        printf("Test failed!\n");
    }
    shutdown();
}