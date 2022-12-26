// alarm.h 
//	Data structures for a software alarm clock.
//
//	We make use of a hardware timer device, that generates
//	an interrupt every X time ticks (on real systems, X is
//	usually between 0.25 - 10 milliseconds).
//
//	From this, we provide the ability for a thread to be
//	woken up after a delay; we also provide time-slicing.
//
//	NOTE: this abstraction is not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ALARM_H
#define ALARM_H

#include "copyright.h"
#include "utility.h"
#include "callback.h"
#include "timer.h"
#include <list>
#include "thread.h"

// TODO, define a data structure to store sleeping thread
class SleepThread{
    public:
        // Methods
        SleepThread(Thread* thread, int x, int count_int);
        Thread* thread;     // store thread that is sleeping
        
        // Variables
        int wakeUpTime;     // when to wake up the thread
};

// TODO, define a manager to cope with sleep threads
class SleepThreadManager{
    public:
        // Methods
        SleepThreadManager();
        void PutToSleep(Thread* thread, int x); // put thread into sleep list
        void CheckAndWakeUp(); // check sleep list and wake up threads
        
        // Variables
        std::list<SleepThread> SleepingThreadList; // store threads that are sleeping
        int count_int; // interrupt counter, increment in CallBack()
};


// The following class defines a software alarm clock. 
class Alarm : public CallBackObj {
  public:
    Alarm(bool doRandomYield);	// Initialize the timer, and callback 
				// to "toCall" every time slice.
    ~Alarm() { delete timer; }
    
    void WaitUntil(int x);	// suspend execution until time > now + x
    
    //TODO, declare manager to store sleep thread information
    SleepThreadManager sleepThreadManager;
    int count_idle;             // count system has idled for how long

  private:
    Timer *timer;		// the hardware timer device

    void CallBack();		// called when the hardware
				// timer generates an interrupt
};

#endif // ALARM_H
