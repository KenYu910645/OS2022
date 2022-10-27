// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

//TODO, implement constructor of SleepThread
SleepThread::SleepThread(Thread* thread, int x, int count_int){
    this->thread = thread;
    this->wakeUpTime = count_int + x;
}

// TODO, put to threads to sleep list
void SleepThreadManager::PutToSleep(Thread* thread, int x){
    // Put this thread into sleep list
    SleepingThreadList.push_back(SleepThread(thread, x, count_int));
    // defined in threads/threads.cc
    thread->Sleep(false);
    cout << "Put thread into sleep when count_int = " << count_int << endl;
}

// TODO, Wake up threads if their wakeUpTime is up
void SleepThreadManager::CheckAndWakeUp(){
    // Increment interrupt counter
    count_int++;
    // Search thread that should be waken 
    for (std::list<SleepThread>::iterator it = SleepingThreadList.begin(); it != SleepingThreadList.end();){
        // Wake up threads
        if (count_int >= it->wakeUpTime){
            kernel->scheduler->ReadyToRun(it->thread);
            cout << "Woke up thread at count_int = " << count_int << endl;
            // pop out threads that are already waken
            it = SleepingThreadList.erase(it);
        }
        else {it++;}
    }
}

// TODO, constructor of SleepThreadManager
SleepThreadManager::SleepThreadManager(){
    count_int = 0;
}


Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
    // TODO, init SleepThreadManager
    sleepThreadManager = SleepThreadManager();
}

// TODO implement Alarm::WaitUntil(x)
void Alarm::WaitUntil(int x){
    // Disable Interrput temporarily. Defined in machine/interrupt.cc
    IntStatus level_tmp = kernel->interrupt->SetLevel(IntOff);
    
    // Get current thread
    Thread* thread_cur = kernel->currentThread;
    
    // Sleep current thread
    sleepThreadManager.PutToSleep(thread_cur, x);
    
    // Enable Interrput
    kernel->interrupt->SetLevel(level_tmp);
}





//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    // TODO, increment counter and check if threads need to wake up
    sleepThreadManager.CheckAndWakeUp(); 
    // TODO, if there's thread still sleeping, don't quit 
    if (status == IdleMode && sleepThreadManager.SleepingThreadList.size() == 0) {	// is it time to quit?
        // TODO OS will quit, only if program has idled from a while
        count_idle++;
        if (!interrupt->AnyFutureInterrupts() and count_idle > 100) {
	    timer->Disable();	// turn off the timer
	}
    } else {			// there's someone to preempt
        count_idle = 0;         // Reset Idle Counter
	interrupt->YieldOnReturn();
    }
}

