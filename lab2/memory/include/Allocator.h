#include "VarSizeAllocMngr.h"
#include <unistd.h>
#include <set>
#include <vector>

class Allocator : public VarSizeAllocMngr
{
public:
    enum class Option : uint8_t
    {
        FF = 0,
        BF,
        WF
    };

    Allocator(int capacity = 1024);
    ~Allocator();

    int AddProcessAndAllocate(int size, Option option = Option::BF);

    int Allocate(int size, Option option = Option::BF);

    void NotDelProcessAndFree(pid_t pid, int offset, int size);

    void DelProcessAndFree(pid_t pid);

    void OutputGraph();

    void Free(pid_t pid, OffsetType Offset, OffsetType Size);

private:
    // father pid : 0
    std::set<int> ChildPidList;
    std::map<int, std::set<int>> Pid2Offsets;  // 记录分配给子进程的 allocation

    static int source;
};
