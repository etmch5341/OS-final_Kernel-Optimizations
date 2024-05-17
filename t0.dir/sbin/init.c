#include "libc.h"

extern unsigned _end;

void handler(int signum, unsigned arg) {
    // printf("*** signal %d 0x%x\n", signum, arg);
    if (signum == 1) {
        /* major page fault */
        void* map_at = (void*)((arg >> 12) << 12);
        void* p = simple_mmap(map_at, 4096, -1, 0);
        if (p != map_at) {
            printf("*** failed to map 0x%x\n", arg);
            shutdown();
        }
    }
    sigreturn();
}

int streq(const char* a, const char* b) {
    int i = 0;

    while (1) {
        char x = a[i];
        char y = b[i];
        if (x != y) return 0;
        if (x == 0) return 1;
        i++;
    }
}

int strlen(const char* str) {
    int n = 0;
    while (*str++ != 0) n++;
    return n;
}

int main(int argc, char** argv) {
    // check raw file for pass/fail details

    simple_signal(handler);

    int test_toggle[] = {
        1,  // automatic sigreturn after handler
        0,  // open, len, and close (absolute)
        0,  // read
        0,  // dup
        0,  // chdir and relative path handling
        0,  // simple mmap with fd
        0   // pipe and write
    };

    int success_count = 0;


    // automatic sigreturn
    if (test_toggle[0]) {
        unsigned p = (unsigned)&_end;
        p = p + 4096;
        p = (p >> 12) << 12;
        unsigned volatile* ptr = (unsigned volatile*)p;
        *ptr = 666;
        success_count++;
    }



    // open len and close with absolute paths
    if (test_toggle[1]) {
        // printf("in test\n");
        int success = 1;

        int fd1 = open("/hello.txt");  // absolute
        // printf("after open\n");

        if (fd1 == -1) {
            success = 0;
            printf("Open failed\n");
        } else {
            if (fd1 != 3) {
                success = 0;
                printf("Open does not allocate minimum fd\n");
            }

            int length = len(fd1);

            if (length != 23) {
                success = 0;
                printf("Length of file returned incorrectly\n");
            }

            if (close(fd1) != 0) {
                success = 0;
                printf("Close failed\n");
            } else {
                if (len(fd1) != -1) {
                    success = 0;
                    printf("Close did not actually close the file\n");
                }
            }
        }

        if (success)
            success_count++;
    }



    // read (assumes open and len is working correctly)
    if (test_toggle[2]) {
        int success = 1;

        int fd = open("/hello.txt");
        int length = len(fd);

        char* buffer = malloc(length + 1);
        buffer[length] = 0;

        int n = read(fd, buffer, length);

        if (n != length) {
            printf("read failed to read any/all bytes\n");
            success = 0;
        } else {
            const char* expected = "*** you can read files\n";
            if (!streq(expected, buffer)) {
                printf("read read incorrectly\n");
                success = 0;
            }
        }

        close(fd);

        if (success)
            success_count++;
    }

    // dup (assumes above works)
    if (test_toggle[3]) {
        int success = 1;

        int fd1 = open("/hello.txt");

        int fd2 = dup(fd1);

        if (fd1 != 3 && fd2 != 4) {
            success = 0;
            printf("Dup and open did not allocate lowest available fd\n");
        } else {

            const char* expected = "*** you can read files\n";

            char* buffer1 = malloc(6);
            buffer1[5] = 0;

            char* buffer2 = malloc(6);
            buffer2[5] = 0;

            if (len(fd1) != len(fd2)) {
                success = 0;
                printf("Unequal sizes between duped files\n");
            }


            read(fd1, buffer1, 5);
            read(fd2, buffer2, 5);

            for (int i = 0; i < 5; i++) {
                if (buffer2[i] != expected[i + 5]) {
                    success = 0;
                    printf("Dup does not share offset\n");
                    break;
                }
            }
        }

        close(fd1);
        close(fd2);

        if (success)
            success_count++;
    }

    // chdir and relative path handling
    if (test_toggle[4]) {
        int success = 1;
        int fd1 = open("hello.txt");
        if (fd1 == -1 || len(fd1) != 23) {
            success = 0;
            printf("relative path not handled correctly\n");
        }


        close(fd1);


        chdir("/data");

        int fd2 = open("hello.txt");
        if (fd2 != -1) {
            success = 0;
            printf("should not see hello from current working directory\n");
            close(fd2);
        }


        int fd3 = open("data.txt");
        if (fd3 == -1) {
            success = 0;
            printf("does not open after change directory correctly\n");
        }


        close(fd3);
        chdir("/");

        if (success)
            success_count++;
    }

    // mmap changes
    if (test_toggle[5]) {
        int success = 1;

        int fd = open("hello.txt");
        int length = len(fd);

        void* addr = simple_mmap(0, 4096, fd, 0);

        char* buffer = malloc(length + 1);
        buffer[length] = 0;
        memcpy(buffer, addr, length);

        const char* expected = "*** you can read files\n";

        if (!streq(expected, buffer)) {
            success = 0;
            printf("mmap does not copy file contents correctly\n");
        }

        close(fd);

        if (success)
            success_count++;
    }

    // pipe and write
    if (test_toggle[6]) {
        // printf("in test\n");
        int success = 1;

        int write_fd = 0;
        int read_fd = 0;
        int rc = pipe(&write_fd, &read_fd);
        // printf("after pipe\n");

        if (rc == -1) {
            success = 0;
            printf("pipe failed\n");
        } else {
            char* w = "*** your pipe works\n";
            int length = strlen(w);

            // printf("after pipe in else statement\n");
            int n = write(write_fd, w, length);
            // printf("after write\n");
            if (n != length) {
                success = 0;
                printf("writing to a buffer failed\n");
            } else {
                // printf("before malloc\n");
                char* r = malloc(length + 1);
                r[length] = 0;
                int n = read(read_fd, r, length);
                // printf("after read\n");

                if (n != length || !streq(w, r)) {
                    success = 0;
                    printf("read from a buffer failed\n");
                }
            }
        }

        

        close(write_fd);
        close(read_fd);

        if (success)
            success_count++;
    }

    int total_tests = 0;
    for (int i = 0; i < 7; i++)
        if (test_toggle[i])
            total_tests++;

    if (total_tests == success_count) {
        printf("*** You passed all selected test cases\n");
    } else {
        printf("*** You passed only %d out of %d test cases\n", success_count, total_tests);
    }

    shutdown();
}
