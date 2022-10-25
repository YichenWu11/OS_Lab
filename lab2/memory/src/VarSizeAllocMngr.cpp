#include <VarSizeAllocMngr.h>
#include <utility>
#include <cassert>
#include <iostream>

VarSizeAllocMngr::OffsetType VarSizeAllocMngr::InvalidOffset = -1;

VarSizeAllocMngr::VarSizeAllocMngr(int capacity) :
    capacity(capacity),
    freeSize(capacity)
{
    // Insert single maximum-size block
    AddNewBlock(0, capacity);
}

VarSizeAllocMngr::VarSizeAllocMngr(VarSizeAllocMngr&& rhs) noexcept :
    m_FreeBlocksByOffset{ std::move(rhs.m_FreeBlocksByOffset) },
    m_FreeBlocksBySize{ std::move(rhs.m_FreeBlocksBySize) },
    capacity{ rhs.capacity },
    freeSize{ rhs.freeSize }
{
    rhs.capacity = 0;
    rhs.freeSize = 0;
}

VarSizeAllocMngr& VarSizeAllocMngr::operator=(VarSizeAllocMngr&& rhs) noexcept {
    m_FreeBlocksByOffset = std::move(rhs.m_FreeBlocksByOffset);
    m_FreeBlocksBySize = std::move(rhs.m_FreeBlocksBySize);
    capacity = rhs.capacity;
    freeSize = rhs.freeSize;

    rhs.capacity = 0;
    rhs.freeSize = 0;

    return *this;
}

void VarSizeAllocMngr::AddNewBlock(OffsetType Offset, OffsetType Size) {
    auto NewBlockIt = m_FreeBlocksByOffset.emplace(Offset, Size);
    auto OrderIt = m_FreeBlocksBySize.emplace(Size, NewBlockIt.first);
    // set the FreeBlockInfo
    NewBlockIt.first->second.OrderBySizeIt = OrderIt;
}


// **********************************************************************************
// Allocate

// FF
VarSizeAllocMngr::OffsetType VarSizeAllocMngr::AllocateFF(OffsetType Size) {
    if(freeSize < Size)
        return InvalidOffset;

    for (const auto& freeBlock : m_FreeBlocksByOffset)
    {
        if (freeBlock.second.Size >= Size)
        {
            auto Offset = freeBlock.first;
            auto NewOffset = Offset + Size;
            auto NewSize = freeBlock.second.Size - Size;
            m_FreeBlocksBySize.erase(freeBlock.second.OrderBySizeIt);
            m_FreeBlocksByOffset.erase(m_FreeBlocksByOffset.find(freeBlock.first));
            if (NewSize > 0)
                AddNewBlock(NewOffset, NewSize);
            freeSize -= Size;
            m_Allocated.emplace(Offset, Size);
            return Offset;
        }
    }

    return InvalidOffset;
}

// BF
VarSizeAllocMngr::OffsetType VarSizeAllocMngr::AllocateBF(OffsetType Size) {
    if(freeSize < Size)
        return InvalidOffset;
 
    // Get the first block that is large enough to encompass Size bytes
    auto SmallestBlockItIt = m_FreeBlocksBySize.lower_bound(Size);
    if(SmallestBlockItIt == m_FreeBlocksBySize.end())
        return InvalidOffset;
 
    auto SmallestBlockIt = SmallestBlockItIt->second;
    auto Offset = SmallestBlockIt->first;
    auto NewOffset = Offset + Size;
    auto NewSize = SmallestBlockIt->second.Size - Size;
    m_FreeBlocksBySize.erase(SmallestBlockItIt);
    m_FreeBlocksByOffset.erase(SmallestBlockIt);
    if (NewSize > 0)
        AddNewBlock(NewOffset, NewSize);
 
    freeSize -= Size;

    m_Allocated.emplace(Offset, Size);
    return Offset;
}

VarSizeAllocMngr::OffsetType VarSizeAllocMngr::AllocateWF(OffsetType Size) {

    auto BiggestBlockItIt = --m_FreeBlocksBySize.end();
    if (BiggestBlockItIt->first < Size)
        return InvalidOffset;

    auto BiggestBlockIt = BiggestBlockItIt->second;
    auto Offset = BiggestBlockIt->first;
    auto NewOffset = Offset + Size;
    auto NewSize = BiggestBlockIt->second.Size - Size;
    m_FreeBlocksBySize.erase(BiggestBlockItIt);
    m_FreeBlocksByOffset.erase(BiggestBlockIt);
    if (NewSize > 0)
        AddNewBlock(NewOffset, NewSize);

    freeSize -= Size;

    m_Allocated.emplace(Offset, Size);
    return Offset;
}

// **********************************************************************************


// 禁止跨块释放
void VarSizeAllocMngr::Free(OffsetType Offset, OffsetType Size)
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
                break;
            }
            else if (Offset > allocation.first && Size == allocation.second)
            {
                //  <--------------------------------------------------------->  all
                //                   <----------------------------------------> To Free
                m_Allocated[allocation.first] -= Size;
                break;
            }
            else if (Offset == allocation.first && Size == allocation.second)
            {
                m_Allocated.erase(m_Allocated.find(allocation.first));
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
                break;
            }
        }
    }
 
    freeSize += Size;
}

void VarSizeAllocMngr::OutputAllocGraph()
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
    std::cout << "   Offset  Size" << std::endl;
    for (const auto& allocation : m_Allocated)
    {
        printf("   %-7d %-10d\n", allocation.first, allocation.second);
    }
    std::cout << "--------------------" << std::endl;
}
