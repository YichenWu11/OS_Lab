#pragma once

#include <Ext2.h>
#include <cstring>
#include <unordered_map>

enum class Command : uint16_t {
    CD = 0,
    MKDIR,
    RMDIR,
    RM,
    TOUCH,
    OPEN,
    CLOSE,
    READ,
    WRITE,
    APPEND,
    LS,
    LL,
    LS_OPEN_FILE,
    CHMOD,
    SHOW_DISK_INFO,
    HELP,
    CLEAR,
    EXIT,
    NONE,
    Count
};

struct Pack {
    Pack() { memset(&(target[0]), 0, sizeof(target)/sizeof(target[0])); }

    bool is_valid{false};
    Command command{Command::NONE};
    char target[9];
};

class Shell {
public:
    static Shell& GetInstance() {
        static Shell instance;
        return instance;
    }

    void init();
    void run();

    Pack inputProcess(std::string input);

private:
    Shell()  = default;
    ~Shell() = default;

    void constructMap();

    void help();

private:
    bool is_init{false};

    std::unordered_map<std::string, Command> input2command;
};
