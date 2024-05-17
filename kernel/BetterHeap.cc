// #include "debug.h"
// #include "stdint.h"
// #include "atomic.h"

// namespace KOTeam{

// struct HeapNode{
//     Atomic<char> bitField;
//     //is occupied: bit 0
//     //right child exists: 1
//     //left child exists: 2
//     //right child coalesce: 3
//     //left child coalesce: 4
// };

// constexpr uint32_t MIN_BLOCK_SIZE = 16;
// constexpr uint32_t max_depth = 22; //logbase2 of 4,194,30;

// /*
// Bytes in heap: 4 * 1024 * 1024 = 4,194,304
// Nodes if min size is 4: 1,048,576
// Pages for just min nodes: 256
// Nodes for 8 byte units:
// Nodes for 16 byte units
// Nodes for 32 byte units:
// 64:
// 128:
// 256:
// 1024:
// 2048:
// 4096:
// 8192:
// 16384:
// 32,768:
// 65,536:
// 131,072:
//  */

// constexpr uint32_t MAX_ARENAS_NUM = 8;

// struct Arena{
//     int* dataRangeStart;
//     int size;
//     SpinLock* lock;
// };

// static int *array;
// static HeapNode *tree;
// static int *index;
// static int depth;
// static int len;
// static int safe = 0;
// static int avail = 0;
// static SpinLock *theLock = nullptr;
// Arena arenas[8];
// Atomic<uint32_t> arenaDistributor(0);


// void makeTaken(int i, int ints);
// void makeAvail(int i, int ints);

// int abs(int x) {
//     if (x < 0) return -x; else return x;
// }

// int size(int i) {
//     return abs(array[i]);
// }

// int headerFromFooter(int i) {
//     return i - size(i) + 1;
// }

// int footerFromHeader(int i) {
//     return i + size(i) - 1;
// }
    
// int sanity(int i) {
//     if (safe) {
//         if (i == 0) return 0;
//         if ((i < 0) || (i >= len)) {
//             Debug::panic("bad header index %d\n",i);
//             return i;
//         }
//         int footer = footerFromHeader(i);
//         if ((footer < 0) || (footer >= len)) {
//             Debug::panic("bad footer index %d\n",footer);
//             return i;
//         }
//         int hv = array[i];
//         int fv = array[footer];
  
//         if (hv != fv) {
//             Debug::panic("bad block at %d, hv:%d fv:%d\n", i,hv,fv);
//             return i;
//         }
//     }

//     return i;
// }

// int left(int i) {
//     return sanity(headerFromFooter(i-1));
// }

// int right(int i) {
//     return sanity(i + size(i));
// }

// int next(int i) {
//     return sanity(array[i+1]);
// }

// int prev(int i) {
//     return sanity(array[i+2]);
// }

// void setNext(int i, int x) {
//     array[i+1] = x;
// }

// void setPrev(int i, int x) {
//     array[i+2] = x;
// }

// void remove(int i) {
//     int prevIndex = prev(i);
//     int nextIndex = next(i);

//     if (prevIndex == 0) {
//         /* at head */
//         avail = nextIndex;
//     } else {
//         /* in the middle */
//         setNext(prevIndex,nextIndex);
//     }
//     if (nextIndex != 0) {
//         setPrev(nextIndex,prevIndex);
//     }
// }

// void makeAvail(int i, int ints) {
//     array[i] = ints;
//     array[footerFromHeader(i)] = ints;    
//     setNext(i,avail);
//     setPrev(i,0);
//     if (avail != 0) {
//         setPrev(avail,i);
//     }
//     avail = i;
// }

// void makeTaken(int i, int ints) {
//     array[i] = -ints;
//     array[footerFromHeader(i)] = -ints;    
// }

// int isAvail(int i) {
//     return array[i] > 0;
// }

// int isTaken(int i) {
//     return array[i] < 0;
// }

// };

// void heapInit(void* base, size_t bytes) {
//     using namespace KOTeam;

//     Debug::printf("| heap range 0x%x 0x%x\n",(uint32_t)base,(uint32_t)base+bytes);

//     /* can't say new becasue we're initializing the heap */

//     int perArenaSize = bytes / MAX_ARENAS_NUM;
//     int start = (int)base;
//     for(int i = 0; i < MAX_ARENAS_NUM; i++){
//         arenas[i].dataRangeStart = (int*)start;
//         int len = perArenaSize / 4;
//         makeTaken(start, 2);
//         makeAvail(start + 2, len - 4);
//         makeTaken(start + len - 2 , 2);
//         start += perArenaSize;
//     }

//     array = (int*) base;
//     len = bytes / 4;
//     makeTaken(0,2);
//     makeAvail(2,len-4);
//     makeTaken(len-2,2);
//     theLock = new SpinLock();
// }

// void* malloc(size_t bytes) {
//     using namespace KOTeam;
//     //Debug::printf("malloc(%d)\n",bytes);



// }

// void free(void* p) {
//     using namespace KOTeam;
//     if (p == 0) return;
//     if (p == (void*) array) return;

    
// }