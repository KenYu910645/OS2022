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
    // Allcoate so many pages is a bit overkill, but if we implement visual memory correctly, this won't be a problem
    // TODO-hw3, initlize pageTable with 1024 entry
    pageTable = new TranslationEntry[MaxNumVirPage];
    for (unsigned int i = 0; i < MaxNumVirPage; i++) {
	pageTable[i].virtualPage = i;
	pageTable[i].physicalPage = i;
        pageTable[i].valid = FALSE;//switch to invalid because system haven't init them
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE; 
    }
    
    // TODO-hw3, initlize swapTable
    for (unsigned int i = 0; i < MaxNumVirPage; i++){
        swapTable[i] = -1; // -1 means, this page is not on disk
    }
    
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // TODO-hw1, need to release physical pages that occupied by process
    for (unsigned int i = 0; i < numPages; i++) {
      	if (kernel->machine->isPhyPageUsed[pageTable[i].physicalPage]){
            kernel->machine->isPhyPageUsed[pageTable[i].physicalPage] = false;
            kernel->machine->numFreePhyPage++;
        }
    }
    // TODO-hw3 recycle disk page used by this process
    for (unsigned int i = 0; i < numPages; i++) {
      	if (kernel->machine->isSwapDiskUsed[swapTable[i]]){
            kernel->machine->isSwapDiskUsed[swapTable[i]] = false;
            kernel->machine->isSecondChancePage[f] = false;
        }
    }
    delete pageTable;
}


// TODO-hw1 Implement translattion
//int AddrSpace::VirtoPhys(int v_addr){
//    return pageTable[v_addr/PageSize].physicalPage*PageSize + v_addr%PageSize; 
//}

void AddrSpace::ReadAtVirutalMem(OpenFile* executable, int segmentSize, int baseVirtualAddr, int inFileAddr){
    // helper function to load code segment and init variable in virtual memory
    // load in page by page
    for (unsigned int i = 0; i < divRoundUp(segmentSize, PageSize); i++){
        int vpn = baseVirtualAddr/PageSize + i; // virtual page number
        if (pageTable[vpn].valid){ // a valid page on memory
            // load page to memory
            executable->ReadAt(&(kernel->machine->mainMemory[pageTablep[vpn].physicalPage*PageSize]), 
	      		      PageSize,
                              inFileAddr + i*PageSize);
        } 
        else{ // invalid page load in virtual memory(SwapDisk)
            char* buffer;
            buffer = new char[PageSize];
            
            //load page content in buffer
            executable->ReadAt(buffer, 
			       PageSize,
                               inFileAddr + i*PageSize);
            // write page to disk(virtual memory)
            kernel->virMemDisk->WriteSector(swapTable[vpn], buffer); 
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
    //cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
    //cout << "noffH.code.size = " << noffH.code.size << endl;
    //cout << "noffH.initData.size = " << noffH.initData.size << endl;
    //cout << "noffH.uninitData.size = " << noffH.uninitData.size << endl;
    //cout << "UserStackSize = " << UserStackSize << endl;
    size = numPages * PageSize;

    // TODO-hw1, check free physical page is enough for this process
    cout << "numFreePhyPage = " << numFreePhyPage << endl;
    cout << "numPages = " << numPages << endl;
    
    // TODO-hw3, switch off physical Memory check 
    if (numPages > numFreePhyPage){
        cout << "numPages larger than numFreePhyPage, except virtual memory implemented." << endl;
    }
    // ASSERT(numPages <= numFreePhyPage);
    ASSERT(numPages <= MaxNumVirPage);
     
    // TODO-hw3, Update pagetable of this process, and only allocate free physical page to it.
    for (unsigned int i = 0; i < numPages; i++) {
      	pageTable[i].virtualPage = i;
        
        // if there are free pages in memory
        if (kernel->machine->numFreePhyPage > 0){
            // Find a free up physical page by linear search
            for (unsigned int j = 0; j < NumPhysPages; j++){
                if (not kernel->machine->isPhyPageUsed[j]){
                    // Allocate physical page to this visual page
                    pageTable[i].physicalPage = j;
                    kernel->machine->isPhyPageUsed[j] = true;
                    kernel->machine->numFreePhyPage--;
                    break;
                };
            }
            pageTable[i].valid = TRUE;           
        }
        else{ // no free page in memory, use virtual memory
            for (unsigned int j = 0; j < MaxNumSwapPage; j++){
                if (not kernel->machine->isSwapDiskUsed[j]){
                    pageTable[i].physicalPage = -1;
                    swapTable[i] = j;
                    kernel->machine->isSwapDiskUsed[j] = true;
                    break;
                }
            }
            pageTable[i].valid = FALSE;
        }
    }
    //cout << "numFreePhyPage after allocate = " << numFreePhyPage << endl;
    
    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// then, copy in the code and data segments into memory
	if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        // TODO-hw3
        ReadAtVirtualMem(executable, noffH.code.size, noffH.code.virtualAddr, noffH.code.inFileAddr);
        // TODO-hw1 need to translate virtualAddr to PhyiscalAddr
        //executable->ReadAt(
	//	&(kernel->machine->mainMemory[VirtoPhys(noffH.code.virtualAddr)]), 
        //			noffH.code.size, noffH.code.inFileAddr);
    }
	if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        // TODO-hw3
        ReadAtVirtualMem(executable, noffH.initData.size, noffH.initData.virtualAddr, noffH.initData.inFileAddr);
        // TODO-hw1 need to translate virtualAddr to PhyiscalAddr
        //executable->ReadAt(
	//	&(kernel->machine->mainMemory[VirtoPhys(noffH.initData.virtualAddr)]),
	//		noffH.initData.size, noffH.initData.inFileAddr);
    }

    delete executable;			// close file
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
    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

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
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
        // TODO-hw3, pass swap table during context switch 
        swapTable=kernel->machine->swapTable;
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
    // TODO-hw3, pass swap table during context switch 
    kernel->machine->swapTable = swapTable;
}
