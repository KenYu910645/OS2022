// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include <string.h>

#define UserStackSize	1024	 	// increase this as necessary!

// TODO-hw3, support 1024 virtual page entry at maximum
#define MaxNumVirPage  1024

class AddrSpace {
  public:
    AddrSpace();			// Create an address space.
    ~AddrSpace();			// De-allocate an address space

    void Execute(char *fileName);	// Run the the program
          // stored in the file "executable"

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
          // for now!
    unsigned int numPages;// Number of pages in the virtual 
          // address space
    
    // TODO-hw3, record virtual pages swap sector 
    unsigned int threadID; // Record which thread is using the address space
    bool pageTable_lock; // Protect pageTable during content switch, only activate before Load() complete
    void ReadAtVirtualMem(OpenFile* executable, int segmentSize, int baseVirtualAddr, int inFileAddr);
    // ReadAtVirtualMem helps to load segment in virtual memory or physical memory

    // TODO-hw1
    // int VirtoPhys(int virtualAddr);     // Translate virtual address to phyiscal 

    bool Load(char *fileName);		// Load the program into memory
          // return false if not found

    void InitRegisters();		// Initialize user-level CPU registers,
          // before jumping to user code
};

#endif // ADDRSPACE_H
