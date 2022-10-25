#pragma once

#include <map>
#include <deque>

// The class handles free memory block management to accommodate variable-size allocation requests.
// It keeps track of free blocks only and does not record allocation sizes. The class uses two ordered maps
// to facilitate operations. The first map keeps blocks sorted by their offsets. The second multimap keeps blocks
// sorted by their sizes. The elements of the two maps reference each other, which enables efficient block
// insertion, removal and merging.
//
//   8                 32                       64                           104
//   |<---16--->|       |<-----24------>|        |<---16--->|                 |<-----32----->|
//
//
//          size2freeblock         offset2freeblock
//           size->offset            offset->size
//
//                16 ------------------>   8 ---------->  {size = 16, &size2freeblock[0]}
//
//                16 ------.   .------->  32 ---------->  {size = 24, &size2freeblock[2]}
//                          '.'
//                24 -------' '-------->  64 ---------->  {size = 16, &size2freeblock[1]}
//
//                32 ------------------> 104 ---------->  {size = 32, &size2freeblock[3]}
//

class VarSizeAllocMngr {
public:
    using OffsetType = int;

protected:
    struct FreeBlockInfo;

    // Type of the map that keeps memory blocks sorted by their offsets
    using TFreeBlocksByOffsetMap = std::map<OffsetType, FreeBlockInfo>;

    // Type of the map that keeps memory blocks sorted by their sizes
    using TFreeBlocksBySizeMap = std::multimap<OffsetType, TFreeBlocksByOffsetMap::iterator>;       

    using AllocatedMap = std::map<OffsetType, OffsetType>; 

    struct FreeBlockInfo {
        // Block size (no reserved space for the size of allocation)
        // actually Size == OrderBySizeIt->first
        OffsetType Size;
        // Iterator referencing this block in the multimap sorted by the block size
        TFreeBlocksBySizeMap::iterator OrderBySizeIt; 

        FreeBlockInfo(OffsetType _Size) : Size(_Size) {}
    };

public:
    VarSizeAllocMngr(OffsetType capacity = 1024);

    VarSizeAllocMngr(VarSizeAllocMngr&&) noexcept;

    VarSizeAllocMngr& operator=(VarSizeAllocMngr&&) noexcept;

    VarSizeAllocMngr(const VarSizeAllocMngr&) = delete;
    VarSizeAllocMngr& operator=(const VarSizeAllocMngr&) = delete;

    void AddNewBlock(OffsetType Offset, OffsetType Size);

    OffsetType AllocateFF(OffsetType Size);
    OffsetType AllocateBF(OffsetType Size);
    OffsetType AllocateWF(OffsetType Size);

    void Free(OffsetType Offset, OffsetType Size);

    void OutputAllocGraph();

    OffsetType GetFreeSize() const { return freeSize; }
    OffsetType GetCapacity() const { return capacity; }

    // InvalidOffset For Allocate
    static OffsetType InvalidOffset;
protected:
    TFreeBlocksByOffsetMap m_FreeBlocksByOffset;
    TFreeBlocksBySizeMap m_FreeBlocksBySize;
    AllocatedMap m_Allocated;

    OffsetType capacity = 0;
    OffsetType freeSize = 0;
};
