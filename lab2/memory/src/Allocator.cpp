#include <Allocator.h>
#include <iostream>

int Allocator::source = 0;

Allocator::Allocator(int capacity) : VarSizeAllocMngr(capacity) {}

Allocator::~Allocator() {}

int Allocator::Allocate(int size, Option option) 
{
    switch (option) {
        case Option::FF:
            return VarSizeAllocMngr::AllocateFF(size);
        case Option::BF:
            return VarSizeAllocMngr::AllocateBF(size);
        case Option::WF:
            return VarSizeAllocMngr::AllocateWF(size);
        default:
            break;
    }

    return -1;
}

int Allocator::AddProcessAndAllocate(int size, Option option)
{
    int pid = ++source;
    auto offset = Allocate(size, option);
    if (offset != InvalidOffset)
    {
        if (Pid2Offsets.find(pid) != Pid2Offsets.end()) Pid2Offsets[pid].emplace(offset);
        else
        {
            Pid2Offsets[pid] = std::set<int>();
            Pid2Offsets[pid].emplace(offset);
        }
        ChildPidList.emplace(pid);
    }
    return offset;
}

void Allocator::DelProcessAndFree(pid_t pid)
{
    if (ChildPidList.find(pid) == ChildPidList.end())
    {
        std::cerr << "Process not exsit!!!" << std::endl;
        return;
    }
    auto &offsetList = Pid2Offsets[pid];
    for (auto &offset : offsetList)
    {
        if (m_Allocated.find(offset) != m_Allocated.end())
        {
            VarSizeAllocMngr::Free(offset, m_Allocated.find(offset)->second);
        }
    }
    ChildPidList.erase(ChildPidList.find(pid));
    Pid2Offsets.erase(Pid2Offsets.find(pid));
}

void Allocator::NotDelProcessAndFree(pid_t pid, int offset, int size)
{
    if (ChildPidList.find(pid) == ChildPidList.end())
    {
        std::cerr << "Process not exsit!!!" << std::endl;
        return;
    }

    Free(pid, offset, size);    
}

void Allocator::Free(pid_t pid, OffsetType Offset, OffsetType Size)
{
    // Find the first element whose offset is greater than the specified offset
    auto NextBlockIt = m_FreeBlocksByOffset.upper_bound(Offset);
    auto PrevBlockIt = NextBlockIt;
    if(PrevBlockIt != m_FreeBlocksByOffset.begin())
        --PrevBlockIt;
    else
        PrevBlockIt = m_FreeBlocksByOffset.end();
    OffsetType NewSize, NewOffset;
    if(PrevBlockIt != m_FreeBlocksByOffset.end() && Offset == PrevBlockIt->first + PrevBlockIt->second.Size)
    {
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //
        NewSize = PrevBlockIt->second.Size + Size;
        NewOffset = PrevBlockIt->first;
 
        if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
        {
            // PrevBlock.Offset           Offset               NextBlock.Offset 
            // |                          |                    |
            // |<-----PrevBlock.Size----->|<------Size-------->|<-----NextBlock.Size----->|
            //
            NewSize += NextBlockIt->second.Size;
            m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
            m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
            // Delete the range of two blocks
            ++NextBlockIt;
            m_FreeBlocksByOffset.erase(PrevBlockIt, NextBlockIt);
        }
        else
        {
            // PrevBlock.Offset           Offset                       NextBlock.Offset 
            // |                          |                            |
            // |<-----PrevBlock.Size----->|<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
            //
            m_FreeBlocksBySize.erase(PrevBlockIt->second.OrderBySizeIt);
            m_FreeBlocksByOffset.erase(PrevBlockIt);
        }
    }
    else if (NextBlockIt != m_FreeBlocksByOffset.end() && Offset + Size == NextBlockIt->first)
    {
        // PrevBlock.Offset                   Offset               NextBlock.Offset 
        // |                                  |                    |
        // |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->|<-----NextBlock.Size----->|
        //
        NewSize = Size + NextBlockIt->second.Size;
        NewOffset = Offset;
        m_FreeBlocksBySize.erase(NextBlockIt->second.OrderBySizeIt);
        m_FreeBlocksByOffset.erase(NextBlockIt);
    }
    else
    {
        // PrevBlock.Offset                   Offset                       NextBlock.Offset 
        // |                                  |                            |
        // |<-----PrevBlock.Size----->| ~ ~ ~ |<------Size-------->| ~ ~ ~ |<-----NextBlock.Size----->|
        //
        NewSize = Size;
        NewOffset = Offset;
    }

    AddNewBlock(NewOffset, NewSize);

    for (auto &allocation : m_Allocated)
    {
        if (Offset >= allocation.first && Size <= allocation.second)
        {
            if (Offset == allocation.first && Size < allocation.second)
            {
                //  <--------------------------------------------------------->  all
                //  <----------------------------------------> To Free
                auto alloc_offset = Offset + Size;
                auto alloc_size   = allocation.second - Size;
                m_Allocated.emplace(alloc_offset, alloc_size);
                m_Allocated.erase(m_Allocated.find(allocation.first));
                Pid2Offsets[pid].erase(Pid2Offsets[pid].find(allocation.first));
                Pid2Offsets[pid].emplace(alloc_offset);
                break;
            }
            else if (Offset > allocation.first && (Offset + Size) == (allocation.first + allocation.second))
            {
                //  <--------------------------------------------------------->  all
                //                   <----------------------------------------> To Free
                m_Allocated[allocation.first] -= Size;
                break;
            }
            else if (Offset == allocation.first && Size == allocation.second)
            {
                m_Allocated.erase(m_Allocated.find(allocation.first));
                Pid2Offsets[pid].erase(Pid2Offsets[pid].find(allocation.first));
                break;
            }
            else
            {
                //  <--------------------------------------------------------->  all
                //           <-------------------------------->  To Free

                auto alloc_offset = Offset + Size;
                auto alloc_size   = allocation.second - Size - (Offset - allocation.first);
                m_Allocated.emplace(alloc_offset, alloc_size);
                m_Allocated[allocation.first] = Offset - allocation.first;
                Pid2Offsets[pid].emplace(alloc_offset);
                break;
            }
        }
    }
 
    freeSize += Size;
}

void Allocator::OutputGraph()
{
    std::cout << std::endl;

    std::cout << "-------[Free]-------" << std::endl;
    std::cout << "   Offset  Size" << std::endl;
    for (const auto& freeBlock : m_FreeBlocksByOffset)
    {
        printf("   %-7d %-10d\n", freeBlock.first, freeBlock.second.Size);
    }
    std::cout << "--------------------" << std::endl;

    std::cout << std::endl;

    std::cout << "-----[Allocated]----" << std::endl;
    std::cout << "Pid   Offset  Size" << std::endl;

    for (auto pid : ChildPidList)
    {
        auto& offsets = Pid2Offsets.find(pid)->second;
        for (auto offset : offsets)
        {
            auto size = m_Allocated[offset];
            printf("%-7d %-6d %-6d\n", pid, offset, size);
        }
    }
    std::cout << "--------------------" << std::endl;
}
