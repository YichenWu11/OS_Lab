#include <sstream>
#include <vector>
#include <cstring>

#include <Shell.h>
#include <Log.h>

void Shell::init() {
    Ext2::GetInstance().init();
    constructMap();
}

void Shell::constructMap() {
    is_init = true;

    input2command["cd"] = Command::CD;              // cd xxx
    input2command["mkdir"] = Command::MKDIR;        // mkdir xxx
    input2command["rmr"] = Command::RMDIR;          // rm -r xxx
    input2command["rm"] = Command::RM;              // rm xxx
    input2command["touch"] = Command::TOUCH;        // touch xxx
    input2command["open"] = Command::OPEN;          // open xxx
    input2command["close"] = Command::CLOSE;        // close xxx
    input2command["read"] = Command::READ;          // read xxx
    input2command["write"] = Command::WRITE;        // write xxx
    input2command["writea"] = Command::APPEND;      // write -a xxx
    input2command["ls"] = Command::LS;              // ls
    input2command["ll"] = Command::LL;              // ll
    input2command["llo"] = Command::LS_OPEN_FILE;   // ll -o
    input2command["lld"] = Command::SHOW_DISK_INFO; // ll -d

    input2command["chmod"] = Command::CHMOD; // chmod xxx

    input2command["help"] = Command::HELP;   // help
    input2command["clear"] = Command::CLEAR; // clear
    input2command["exit"] = Command::EXIT;   // exit
    input2command["quit"] = Command::EXIT;   // quit
}

Pack Shell::inputProcess(std::string input) {
    std::string space_delimiter = " ";
    std::vector<std::string> inputs{};

    size_t pos = 0;
    while ((pos = input.find(space_delimiter)) != std::string::npos) {
        inputs.push_back(input.substr(0, pos));
        input.erase(0, pos + space_delimiter.length());
    }
    inputs.push_back(input.substr(0, pos));

    assert(inputs.size() <= 3);

    std::string process_input;
    if (inputs.size() == 3)
        process_input = inputs[0] + inputs[1].substr(1, 1);
    else if (inputs.size() == 2)
        if (inputs[1][0] == '-')
            process_input = inputs[0] + inputs[1].substr(1, 1);
        else
            process_input = inputs[0];
    else if (inputs.size() == 1)
        process_input = inputs[0];

    Pack res;

    if (input2command.find(process_input) != input2command.end()) {
        res.command = input2command[process_input];
        res.is_valid = true;
        if (inputs.size() == 3) {
            if (inputs[2].length() <= 9)
                strncpy(&(res.target[0]), inputs[2].data(), inputs[2].length());
            else {
                res.is_valid = false;
                ERROR("The max length of name of file/dir is 9!!!\n");
            }
        }
        else if (inputs.size() == 2)
            if (inputs[1][0] != '-') {
                if (inputs[1].length() <= 9)
                    strncpy(&(res.target[0]), inputs[1].data(), inputs[1].length());
                else {
                    res.is_valid = false;
                    ERROR("The max length of name of file/dir is 9!!!\n");
                }
            }
    }
    else {
        res.is_valid = false;
    }

    return res;
}

void Shell::help() {
    INFO("\n----------------- command --------------------\n");
    INFO("cd    [--]  xxx : change current directory to xxx\n");
    INFO("mkdir [--]  xxx : make a new directory named xxx in current directory\n");
    INFO("rm    [-r]  xxx : remove a file(without -r) // remove a directory(with -r)\n");
    INFO("touch [--]  xxx : create a new file named xxx in current directory\n");
    INFO("open  [--]  xxx : add the file named xxx to file_open_table\n");
    INFO("close [--]  xxx : remove the file named xxx from file_open_table\n");
    INFO("read  [--]  xxx : output the content of file in the terminal\n");
    INFO("write [-a]  xxx : delete original content then write(without -a) // Append write(with -a)\n");
    INFO("ls    [--]      : list the contents in current didirectoryr\n");
    INFO("ll    [-od]     : list details // show disk info(with -d) // list file open table(with -o)\n");
    INFO("chmod [--]  xxx : change the mode of the file\n");
    INFO("clear [--]      : clear\n");
    INFO("exit  [--]      : exit\n");
    INFO("----------------- command --------------------\n\n");
}

void Shell::run() {
    assert(is_init && "Init first!!!");

    while (1) {
        Ext2::GetInstance().printCurrPath();

        std::cin.clear();
        fflush(stdin);

        std::string input;
        std::getline(std::cin, input);

        Pack res = this->inputProcess(input);

        if (!res.is_valid) {
            ERROR("Input Error!!! Please check your input!!!\n");
            continue;
        }

        switch (res.command) {
        case Command::CD:
            Ext2::GetInstance().ext2_cd(res.target);
            break;
        case Command::MKDIR:
            Ext2::GetInstance().ext2_mkdir(res.target);
            break;
        case Command::RMDIR:
            Ext2::GetInstance().ext2_rmdir(res.target);
            break;
        case Command::RM:
            Ext2::GetInstance().ext2_rm(res.target);
            break;
        case Command::TOUCH:
            Ext2::GetInstance().ext2_touch(res.target);
            break;
        case Command::OPEN:
            Ext2::GetInstance().ext2_open(res.target);
            break;
        case Command::CLOSE:
            Ext2::GetInstance().ext2_close(res.target);
            break;
        case Command::READ:
            Ext2::GetInstance().ext2_read(res.target);
            break;
        case Command::WRITE:
            Ext2::GetInstance().ext2_write(res.target);
            break;
        case Command::APPEND:
            Ext2::GetInstance().ext2_append(res.target);
            break;
        case Command::LS:
            Ext2::GetInstance().ext2_ls();
            break;
        case Command::LL:
            Ext2::GetInstance().ext2_ll();
            break;
        case Command::LS_OPEN_FILE:
            Ext2::GetInstance().ext2_l_open_file();
            break;
        case Command::CHMOD:
            Ext2::GetInstance().ext2_chmod(res.target);
            break;
        case Command::SHOW_DISK_INFO:
            Ext2::GetInstance().showDiskInfo();
            break;
        case Command::HELP:
            help();
            break;
        case Command::CLEAR:
            system("clear");
            break;
        case Command::EXIT:
            exit(0);
            break;
        default:
            break;
        }
    }
}
