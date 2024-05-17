#include "file.h"

#include "process.h"

namespace UserFileIO {

// +++ UserFile

UserFile::~UserFile() {}

int64_t UserFile::len() {
    return -1;
}

UserFileContainer::UserFileContainer(UserFile* ptr) : ptr(ptr) {}

UserFileContainer::~UserFileContainer() {
    delete ptr;
}

// +++ OpenFile

OpenFile::OpenFile() : OpenFile(Shared<UserFileContainer>::NUL, 0) {}

OpenFile::OpenFile(Shared<UserFileContainer> uf, Flags perms) : uf(uf), perms(perms) {}

UserFileType OpenFile::type() {
    return uf->ptr->type();
}

int64_t OpenFile::len() {
    return uf->ptr->len();
}

int64_t OpenFile::read(uint32_t len, void* buffer) {
    if (perms.is_not(Flags::USER_FILE_READ)) {
        return {-1};
    }
    return uf->ptr->do_read(len, buffer);
}

int64_t OpenFile::write(uint32_t len, void* buffer) {
    if (perms.is_not(Flags::USER_FILE_WRITE)) {
        return {-1};
    }
    return uf->ptr->do_write(len, buffer);
}

// +++ NodeFile

NodeFile::NodeFile(Shared<Node> node) : guard(),
                                        offset(0),
                                        node(node) {}
NodeFile::~NodeFile() {}

UserFileType NodeFile::type() {
    return NODE;
}

int64_t NodeFile::do_read(uint32_t len, void* buffer) {
    using namespace VMM;

    if (node->is_dir()) {
        return {-1};
    }

    LockGuard{guard};

    // int64_t nbyte = BadPageCache::read_all(node, offset, len, buffer);
    int64_t nbyte = node->read_all(offset, len, (char*)buffer);

    if (nbyte != -1) {
        offset += nbyte;
    }

    return nbyte;
}

int64_t NodeFile::do_write(uint32_t len, void* buffer) {
    // NOTE : writes are disabled ... for now
    return -1;
}

int64_t NodeFile::len() {
    if (node->is_dir()) {
        return {-1};
    }

    return node->size_in_bytes();
}

// +++ TerminalFile

TerminalFile::~TerminalFile() {}

UserFileType TerminalFile::type() {
    return TERMINAL;
}

int64_t TerminalFile::do_read(uint32_t len, void* buffer) {
    // NOTE : reading is not allowed for now
    return -1;
}

int64_t TerminalFile::do_write(uint32_t len, void* buffer) {
    char* bytes = (char*)buffer;
    for (uint32_t i = 0; i < len; i++) {
        Debug::printf("%c", bytes[i]);
    }
    return len;
}

// +++ PipeFile

PipeFile::PipeFile() : bb(100) {}
PipeFile::~PipeFile() {}

UserFileType PipeFile::type() {
    return PIPE;
}

int64_t PipeFile::do_read(uint32_t len, void* buffer) {
    using namespace ProcessManagement;

    /**
     * create the trigger... we need one extra spots
     * we need one for each byte to read and one for the trigger
     */
    Shared<Barrier> trigger = Shared<Barrier>::make(len + 1);

    for (uint32_t i = 0; i < len; i++) {
        char* place = ((char*)buffer) + i;
        bb.get([trigger, place, me = Process::current()](char c) {
            // FIXME : this is horibbly slow... we should either write a custom BB or buffer the contents
            Process old = Process::change(me);
            *place = c;
            Process::change(old);
            trigger->sync();
        });
    }

    PCB::current().regs.eax = len;
    block_and_sync(trigger);

    // let's return -2 here so we can see a sign if something is wrong
    // it should never return
    return -2;
}

int64_t PipeFile::do_write(uint32_t len, void* buffer) {
    using namespace ProcessManagement;

    /**
     * create the trigger... we need one extra spots
     * we need one for each byte to write and one for the trigger
     */
    Shared<Barrier> trigger = Shared<Barrier>::make(len + 1);

    for (uint32_t i = 0; i < len; i++) {
        char* place = ((char*)buffer) + i;
        bb.put(*place, [trigger]() {
            trigger->sync();
        });
    }

    PCB::current().regs.eax = len;
    block_and_sync(trigger);

    // let's return -2 here so we can see a sign if something is wrong
    // it should never retur
    return -2;
}

void PipeFile::block_and_sync(Shared<Barrier>& trigger) {
    using namespace ProcessManagement;

    // block here
    block([&trigger](Process me) {
        // only allow the schedule of the process when we have safely disconnected from the process PD
        trigger->sync([me] {
            me.schedule();
        });

        // consume the barrier
        trigger = Shared<Barrier>::NUL;
    });
}

// globals

Shared<UserFileContainer> terminal{};
OpenFile stdin{};
OpenFile stdout{};
OpenFile stderr{};

void init_user_file_io() {
    terminal = Shared<UserFileContainer>::make(new TerminalFile());
    stdin = OpenFile(terminal, 0);
    stdout = OpenFile(terminal, Flags::USER_FILE_WRITE);
    stderr = OpenFile(terminal, Flags::USER_FILE_WRITE);
}

}  // namespace UserFileIO