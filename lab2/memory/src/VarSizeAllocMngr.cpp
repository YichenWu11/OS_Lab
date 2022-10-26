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
std::vector<VarSizeAllocMngr::OffsetType> VarSizeAllocMngr::AllocateFF(OffsetType Size) {
    if(freeSize < Size)
        return {InvalidOffset};

    auto BiggestBlockItIt = --m_FreeBlocksBySize.end();

    if (BiggestBlockItIt->first < Size)
    {
        // shrink
        auto RestSize = Size;
        auto Offset = 0;
        std::vector<OffsetType> res;

        auto NewOffset = 0;
        auto NewSize   = 0;

        bool isInit = false;

        std::vector<OffsetType> offset2erase;

        for (const auto& freeBlock : m_FreeBlocksByOffset)
        {
            if (RestSize > freeBlock.second.Size)
            {
                isInit = true;
                res.push_back(freeBlock.first);
                m_Allocated.emplace(freeBlock.first, freeBlock.second.Size);
                m_FreeBlocksBySize.erase(freeBlock.second.OrderBySizeIt);
                offset2erase.push_back(freeBlock.first);
            }
            else
            {
                isInit = true;
                res.push_back(freeBlock.first);
                NewOffset = freeBlock.first + RestSize;
                NewSize = freeBlock.second.Size - RestSize;
                m_Allocated.emplace(freeBlock.first, RestSize);
                m_FreeBlocksBySize.erase(freeBlock.second.OrderBySizeIt);
                offset2erase.push_back(freeBlock.first);
            }
            RestSize -= freeBlock.second.Size;
        }

        if (isInit)
        {
            for (auto &off : offset2erase) m_FreeBlocksByOffset.erase(m_FreeBlocksByOffset.find(off));
            if (NewSize > 0)
            {
                m_FreeBlocksByOffset.emplace(NewOffset, NewSize);
                auto OrderIt = m_FreeBlocksBySize.emplace(NewSize, m_FreeBlocksByOffset.find(NewOffset));
                m_FreeBlocksByOffset.find(NewOffset)->second.OrderBySizeIt = OrderIt;
            }
            return res;
        }
        else
            return {InvalidOffset};
    }
    else /* not shrink */
    {
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
                return {Offset};
            }
        }
    }
    return {InvalidOffset};
}

// BF
std::vector<VarSizeAllocMngr::OffsetType> VarSizeAllocMngr::AllocateBF(OffsetType Size) {
    if(freeSize < Size)
        return {InvalidOffset};

    auto BiggestBlockItIt = --m_FreeBlocksBySize.end();

    if (BiggestBlockItIt->first < Size)
    {
        // shrink
        auto RestSize = Size;
        auto Offset = 0;
        std::vector<OffsetType> res;

        auto NewOffset = 0;
        auto NewSize   = 0;

        bool isInit = false;

        std::vector<TFreeBlocksBySizeMap::iterator> iter2erase;

        for (const auto& freeBlock : m_FreeBlocksBySize)
        {
            if (RestSize > freeBlock.first)
            {
                isInit = true;
                res.push_back(freeBlock.second->first);
                m_Allocated.emplace(freeBlock.second->first, freeBlock.first);
                iter2erase.push_back(freeBlock.second->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(freeBlock.second);
            }
            else
            {
                isInit = true;
                res.push_back(freeBlock.second->first);
                NewOffset = freeBlock.second->first + RestSize;
                NewSize = freeBlock.first - RestSize;
                m_Allocated.emplace(freeBlock.second->first, RestSize);
                iter2erase.push_back(freeBlock.second->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(freeBlock.second);
            }
            RestSize -= freeBlock.first;
        }

        if (isInit)
        {
            for (auto &iter : iter2erase) m_FreeBlocksBySize.erase(iter);
            if (NewSize > 0)
            {
                m_FreeBlocksByOffset.emplace(NewOffset, NewSize);
                auto OrderIt = m_FreeBlocksBySize.emplace(NewSize, m_FreeBlocksByOffset.find(NewOffset));
                m_FreeBlocksByOffset.find(NewOffset)->second.OrderBySizeIt = OrderIt;
            }
            return res;
        }
        else
            return {InvalidOffset};
    }
    else
    {
        // Get the first block that is large enough to encompass Size bytes
        auto SmallestBlockItIt = m_FreeBlocksBySize.lower_bound(Size);
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
        return {Offset};
    }
    return {InvalidOffset};
}

std::vector<VarSizeAllocMngr::OffsetType> VarSizeAllocMngr::AllocateWF(OffsetType Size) {

    auto BiggestBlockItIt = --m_FreeBlocksBySize.end();

    if (BiggestBlockItIt->first < Size)
    {
        // shrink
        auto RestSize = Size;
        auto Offset = 0;
        std::vector<OffsetType> res;

        auto NewOffset = 0;
        auto NewSize   = 0;

        bool isInit = false;

        std::vector<TFreeBlocksBySizeMap::iterator> iter2erase;

        for (auto iter = m_FreeBlocksBySize.rbegin(); iter != m_FreeBlocksBySize.rend(); ++iter)
        {
            if (RestSize > iter->first)
            {
                isInit = true;
                res.push_back(iter->second->first);
                m_Allocated.emplace(iter->second->first, iter->first);
                iter2erase.push_back(iter->second->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(iter->second);
            }
            else
            {
                isInit = true;
                res.push_back(iter->second->first);
                NewOffset = iter->second->first + RestSize;
                NewSize = iter->first - RestSize;
                m_Allocated.emplace(iter->second->first, RestSize);
                iter2erase.push_back(iter->second->second.OrderBySizeIt);
                m_FreeBlocksByOffset.erase(iter->second);
            }
            RestSize -= iter->first;
        }

        if (isInit)
        {
            for (auto &iter : iter2erase) m_FreeBlocksBySize.erase(iter);
            if (NewSize > 0)
            {
                m_FreeBlocksByOffset.emplace(NewOffset, NewSize);
                auto OrderIt = m_FreeBlocksBySize.emplace(NewSize, m_FreeBlocksByOffset.find(NewOffset));
                m_FreeBlocksByOffset.find(NewOffset)->second.OrderBySizeIt = OrderIt;
            }
            return res;
        }
        else
            return {InvalidOffset};       
    }
    else
    {
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
        return {Offset};
    }
    return {InvalidOffset};
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
        if (Offset >= allocation.first && Size <= allocation.second && Offset + Size <= allocation.first + allocation.second)
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
            else if (Offset > allocation.first && Offset + Size == allocation.first + allocation.second)
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
