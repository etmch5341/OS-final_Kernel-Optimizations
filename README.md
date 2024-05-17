# os-final

## Guidelines
DO NOT PUSH TO MAIN BRANCH


There is a branch for each group. Here are the branch names:
 - authsecurity
 - dynamiclinking
 - filesystem
 - graphics
 - kerneloptims
 - keymouse
 - libc
 - main
 - python
 - shell
 - signals
 - sound
 - texteditor
 - vmm
 - windowing

Project tutorial (credits to Andy): https://docs.google.com/document/d/18X6Z5hzBp_Adb_Kr06RXS1P2jYYWlZGpvNtL05Eqtq0/edit

Before you start working make sure that you checkout to your branch:
```git checkout [branch name]```
To pull changes from the main branch as you work, you can do ```git pull origin main``` while you are checked out to your branch. Once you have finalized some changes, you push your changes to you branch with ```git push origin [branch name]``` and go online to make a pull request to merge into main and someone will review and handle it.


Check out the issues and projects tab to update your status and see what needs to get done.


Other than that, let's try to have fun :)



### Initial comments by Ojas
I realize this may not be helpful cuz its brief oops oh well
Also I dont think you need to know any of this because we need to decide on standard interfaces between teams, but here it is anyways
Here is a list of some of the interfaces / data structures I have:
- genericds
  - LinkedList<T, LockType>: a linked list of any type with any lock type
    - LinkedList<uint32_t, SpinLock>(0) creates a linked list protected by a spin lock with null values returns as 0
    - LinkedList<uint32_t*, NoLock>(nullptr) creates a linked list protected by no lock with null values as nullptr
  - FixedHashMap<K, V, LockType>: a hashmap with a fixed number of buckets.
    - Depends on the struct EqualFunction<K> and struct HashFunction<K>
    - also takes in a null value like the linked list
  - RBTree<T, LockType>: a red-black tree protected by specified locktype
    - similar null value situation
    - depends on the struct CompareFunction<K>
    - supports identifier-based search and removal (i.e. you can use a different type than the RBTree specified to search or remove from it, just specify a comparator. this is very useful if you want to use multiple types of keys for the same data)
  - you can find examples of the Function types throughout vmm.cc or ask me lol
- libk
  - streq, strcmp, strcpy, etc.
  - min and max
- flags
  - is a 4 bytes set of flags that supports set operations
  - flag union is through |
  - flag intersection is through &
  - flag difference is through -
  - flag negation is through ~
  - flag is_superset is through is()
  - is_not() is congruent to !is() and is only for readability
  - see vmm.cc to see examples or ask me lol
- process
  - the PCB is mapped in the virtual address space at VA_PROCESS
  - PCB::current() gets a reference to the current pcb
  - proc.pcb_phys() returns the physical references to the pcb for the process
  - Process::current() gets the current process
  - Process::change(Process other) changes the current process
  - Process::create_like(Process other) creates a new process with the same mappings as the given one
  - Process::destroy(Process other) destroys a process
  - no need to worry about deallocating frames as everything is handled automatically by the vmm system
- if u need to read from a file, use mmap if possible. i have defined mmap like this
  - void* mmap(RBTree<MMAPBlock *, NoLock> *mmap_tree, VirtualAddress va, uint32_t length, Flags flags, Shared<Node> file, uint32_t file_offset, uint32_t file_size)
  - va must be page aligned
  - file_offset must be page aligned unless you pass in the Flags::MMAP_F_UNALGN
  - file_size is only used if you truncate the file with Flags::MMAP_F_TRUNC
