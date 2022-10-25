## WF

```cpp
    VarSizeAllocMngr allocMngr;

    allocMngr.AllocateBF(10);

    allocMngr.AllocateBF(900);

    allocMngr.Free(10, 200);
    allocMngr.Free(400, 300);

    allocMngr.OutputAllocGraph();

    allocMngr.AllocateWF(50);

    allocMngr.OutputAllocGraph();
```