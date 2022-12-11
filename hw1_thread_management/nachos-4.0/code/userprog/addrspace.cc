// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

// TODO-hw1, Initialize Data structure that help record status of physical memory
// bool AddrSpace::isPhyPageUsed[NumPhysPages] = {false};
// unsigned int AddrSpace::numFreePhyPage = NumPhysPages;

AddrSpace::AddrSpace()
{
    // TODO-hw3, assign thread ID to each thread by a static counter 
    static int threadCounter = 0;
    threadID = threadCounter;
    threadCounter++;
    cout << "Initializing thread AddrSpace, threadID = " << threadID << endl;

    // TODO-hw3, initlize pageTable with MaxNumVirPage entry
    pageTable = new TranslationEntry[MaxNumVirPage];
    for (unsigned int i = 0; i < MaxNumVirPage; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = i;
        pageTable[i].valid = FALSE; //switch to invalid because system haven't init them
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
        pageTable[i].swapSectorId = -1; //default no page is on disk
    }

    // register threadID pagetable in machine
    kernel->machine->pageTableAll[threadID] = pageTable;

    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // TODO-hw1, release physical pages that were occupied by this thread
    for (unsigned int i = 0; i < numPages; i++) {
          if (kernel->machine->frameTable.t[pageTable[i].physicalPage].useThreadID == threadID){
            kernel->machine->frameTable.t[pageTable[i].physicalPage].useThreadID = -1;
        }
    }
    // TODO-hw3, release disk pages that were occupied by this thread
    for (unsigned int i = 0; i < numPages; i++) {
          if (kernel->machine->isSwapDiskUsed[pageTable[i].swapSectorId]){
            kernel->machine->isSwapDiskUsed[pageTable[i].swapSectorId] = false;
        }
    }
    delete pageTable;
}


// TODO-hw1 Implement translattion
//int AddrSpace::VirtoPhys(int v_addr){
//    return pageTable[v_addr/PageSize].physicalPage*PageSize + v_addr%PageSize; 
//}

void AddrSpace::ReadAtVirtualMem(OpenFile* executable, int segmentSize, int baseVirtualAddr, int inFileAddr){
    // helper function to load code segment and init variable in virtual memory
    // load in page by page
    
    cout << "[addresspace.cc] ReadAtVirtualMem claim " << divRoundUp(segmentSize, PageSize) << " pages" << endl;
    for (unsigned int i = 0; i < divRoundUp(segmentSize, PageSize); i++){
        int vpn = baseVirtualAddr/PageSize + i; // virtual page number
        // If there is enough memory for paging, claim a page for this thread
        if (kernel->machine->frameTable.getNumFreeFrame() > 0){
            // Find a free frame
            int freeFrameNum = kernel->machine->frameTable.getFreeFrameNum();

            // load page to memory
            executable->ReadAt(&(kernel->machine->mainMemory[freeFrameNum*PageSize]), 
                                 PageSize,
                               inFileAddr + i*PageSize);
            // Update pageTable
            pageTable[vpn].virtualPage  = vpn;
            pageTable[vpn].physicalPage = freeFrameNum;
            pageTable[vpn].swapSectorId = -1;
            pageTable[vpn].valid = TRUE;

            // Update FrameTable
            kernel->machine->frameTable.t[freeFrameNum].refBit = false;
            kernel->machine->frameTable.t[freeFrameNum].useThreadID = threadID;
            kernel->machine->frameTable.t[freeFrameNum].virtualPageNum = vpn;   
            
            cout << "[addrespace.cc] vpn " << vpn << " map to frame " << freeFrameNum << endl;
        }
        else{ // invalid page load in virtual memory(SwapDisk)
            char* buffer = new char[PageSize];
            // find a free disk sector for virtual memory
            int sectorId = kernel->machine->getFreeDiskSect();
            // Update pageTable
            pageTable[vpn].virtualPage  = vpn;
            pageTable[vpn].physicalPage = -1;
            pageTable[vpn].swapSectorId = sectorId;
            pageTable[vpn].valid = FALSE;

            // Update SectorTable
            kernel->machine->isSwapDiskUsed[sectorId] = true;
            
            //load page content in buffer
            executable->ReadAt(buffer, 
                               PageSize,
                               inFileAddr + i*PageSize); 
            // write page to disk(virtual memory)
            kernel->virMemDisk->WriteSector(sectorId, buffer);
            delete[] buffer;
            cout << "[addrespace.cc] vpn " << vpn << " map to sector " << sectorId << endl;
        }
    }
}

//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
    cerr << "Unable to open file " << fileName << "\n";
    return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;	// we need to increase the size
                        // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    // cout << "noffH.code.size = " << noffH.code.size << endl;
    // cout << "noffH.initData.size = " << noffH.initData.size << endl;
    // cout << "noffH.uninitData.size = " << noffH.uninitData.size << endl;
    // cout << "UserStackSize = " << UserStackSize << endl;
    size = numPages * PageSize;

    // TODO-hw3
    cout << "numFreePhyPage = " << kernel->machine->frameTable.getNumFreeFrame() << endl;
    cout << "numPages = " << numPages << endl;
    
    // TODO-hw3, switch off physical Memory check 
    if (numPages > kernel->machine->frameTable.getNumFreeFrame()){
        cout << "numPages larger than numFreePhyPage, expect virtual memory is implemented." << endl;
    }
    // Make sure we have enough virtual address for this thread
    ASSERT(numPages <= MaxNumVirPage);

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
        DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        // TODO-hw3, initialize the adddress space of the whole thread in one go
        ReadAtVirtualMem(executable, size, noffH.code.virtualAddr, noffH.code.inFileAddr);

        // TODO-hw1 need to translate virtualAddr to PhyiscalAddr
        //executable->ReadAt(
    //	&(kernel->machine->mainMemory[VirtoPhys(noffH.code.virtualAddr)]), 
        //			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
    DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        // TODO-hw1 need to translate virtualAddr to PhyiscalAddr
        //executable->ReadAt(
    //	&(kernel->machine->mainMemory[VirtoPhys(noffH.initData.virtualAddr)]),
    //		noffH.initData.size, noffH.initData.inFileAddr);
    }
    delete executable;			// close file
    cout << "[addrespace.cc] Successfully initalize thread "<< threadID << " address space." << endl;
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    // TODO-hw3, acquire lock, prevent content swtich change the page table
    pageTable_lock = true;
    
    if (!Load(fileName)) {
    cout << "inside !Load(FileName)" << endl;
    return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register
    
    // TODO-hw3, release lock
    pageTable_lock = false;
    
    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
                    // the address space exits
                    // by doing the syscall "exit"
}


//----------------------------------------------------------------------

// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    // TODO-hw3, use lock to prevent OS override pageTable during Loading
    if (!pageTable_lock){
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
    }
    // cout << "Save State for "<< threadID << endl;   
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
    // TODO-hw3, tell machine which thread is running now
    kernel->machine->threadID = threadID;
    // cout << "Restore State for "<< threadID << endl;
}
