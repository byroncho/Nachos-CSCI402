// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#ifdef CHANGED
#include "synch.h"
#include <stdlib.h>
#endif

#ifdef CHANGED
// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");  // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
  t1_l1.Acquire();
  t1_s1.V();  // Allow t1_t2 to try to Acquire Lock

  printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
   t1_l1.getName());
  t1_s3.P();
  printf ("%s: working in CS\n",currentThread->getName());
  for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
     t1_l1.getName());
  t1_l1.Release();
  t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

  t1_s1.P();// Wait until t1 has the lock
  t1_s2.V();  // Let t3 try to acquire the lock

  printf("%s: trying to acquire lock %s\n",currentThread->getName(),
    t1_l1.getName());
  t1_l1.Acquire();

  printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
   t1_l1.getName());
  for (int i = 0; i < 10; i++)
    ;
  printf ("%s: Releasing Lock %s\n",currentThread->getName(),
   t1_l1.getName());
  t1_l1.Release();
  t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

  t1_s2.P();// Wait until t2 is ready to try to acquire the lock

  t1_s3.V();// Let t1 do it's stuff
  for ( int i = 0; i < 3; i++ ) {
    printf("%s: Trying to release Lock %s\n",currentThread->getName(),
      t1_l1.getName());
    t1_l1.Release();
  }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");// For mutual exclusion
Condition t2_c1("t2_c1");// The condition variable to test
Semaphore t2_s1("t2_s1",0);// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
// done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
  t2_l1.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
    t2_l1.getName(), t2_c1.getName());
  t2_c1.Signal(&t2_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
    t2_l1.getName());
  t2_l1.Release();
  t2_s1.V();// release t2_t2
  t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
  t2_s1.P();// Wait for t2_t1 to be done with the lock
  t2_l1.Acquire();
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
    t2_l1.getName(), t2_c1.getName());
  t2_c1.Wait(&t2_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
    t2_l1.getName());
  t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");// For mutual exclusion
Condition t3_c1("t3_c1");// The condition variable to test
Semaphore t3_s1("t3_s1",0);// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
  t3_l1.Acquire();
  t3_s1.V();// Let the signaller know we're ready to wait
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
    t3_l1.getName(), t3_c1.getName());
  t3_c1.Wait(&t3_l1);
  printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
  t3_l1.Release();
  t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

  // Don't signal until someone's waiting

  for ( int i = 0; i < 5 ; i++ ) 
    t3_s1.P();
  t3_l1.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
    t3_l1.getName(), t3_c1.getName());
  t3_c1.Signal(&t3_l1);
  printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
  t3_l1.Release();
  t3_done.V();
}

// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");// For mutual exclusion
Condition t4_c1("t4_c1");// The condition variable to test
Semaphore t4_s1("t4_s1",0);// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
  t4_l1.Acquire();
  t4_s1.V();// Let the signaller know we're ready to wait
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
    t4_l1.getName(), t4_c1.getName());
  t4_c1.Wait(&t4_l1);
  printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
  t4_l1.Release();
  t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

  // Don't broadcast until someone's waiting

  for ( int i = 0; i < 5 ; i++ ) 
    t4_s1.P();
  t4_l1.Acquire();
  printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
    t4_l1.getName(), t4_c1.getName());
  t4_c1.Broadcast(&t4_l1);
  printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
  t4_l1.Release();
  t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");// For mutual exclusion
Lock t5_l2("t5_l2");// Second lock for the bad behavior
Condition t5_c1("t5_c1");// The condition variable to test
Semaphore t5_s1("t5_s1",0);// To make sure t5_t2 acquires the lock after
// t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
  t5_l1.Acquire();
  t5_s1.V();// release t5_t2
  printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
    t5_l1.getName(), t5_c1.getName());
  t5_c1.Wait(&t5_l1);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
    t5_l1.getName());
  t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
  t5_s1.P();// Wait for t5_t1 to get into the monitor
  t5_l1.Acquire();
  t5_l2.Acquire();
  printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
    t5_l2.getName(), t5_c1.getName());
  t5_c1.Signal(&t5_l2);
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
    t5_l2.getName());
  t5_l2.Release();
  printf("%s: Releasing Lock %s\n",currentThread->getName(),
    t5_l1.getName());
  t5_l1.Release();
}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
// 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
  Thread *t;
  char *name;
  int i;

  // Test 1

  printf("Starting Test 1\n");

  t = new Thread("t1_t1");
  t->Fork((VoidFunctionPtr)t1_t1,0);

  t = new Thread("t1_t2");
  t->Fork((VoidFunctionPtr)t1_t2,0);

  t = new Thread("t1_t3");
  t->Fork((VoidFunctionPtr)t1_t3,0);

  // Wait for Test 1 to complete
  for (  i = 0; i < 2; i++ )
    t1_done.P();

  // Test 2

  printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
  printf("completes\n");

  t = new Thread("t2_t1");
  t->Fork((VoidFunctionPtr)t2_t1,0);

  t = new Thread("t2_t2");
  t->Fork((VoidFunctionPtr)t2_t2,0);

  // Wait for Test 2 to complete
  t2_done.P();

  // Test 3

  printf("Starting Test 3\n");

  for (  i = 0 ; i < 5 ; i++ ) {
    name = new char [20];
    sprintf(name,"t3_waiter%d",i);
    t = new Thread(name);
    t->Fork((VoidFunctionPtr)t3_waiter,0);
  }
  t = new Thread("t3_signaller");
  t->Fork((VoidFunctionPtr)t3_signaller,0);

  // Wait for Test 3 to complete
  for (  i = 0; i < 2; i++ )
    t3_done.P();

  // Test 4

  printf("Starting Test 4\n");

  for (  i = 0 ; i < 5 ; i++ ) {
    name = new char [20];
    sprintf(name,"t4_waiter%d",i);
    t = new Thread(name);
    t->Fork((VoidFunctionPtr)t4_waiter,0);
  }
  t = new Thread("t4_signaller");
  t->Fork((VoidFunctionPtr)t4_signaller,0);

  // Wait for Test 4 to complete
  for (  i = 0; i < 6; i++ )
    t4_done.P();

  // Test 5

  printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
  printf("completes\n");

  t = new Thread("t5_t1");
  t->Fork((VoidFunctionPtr)t5_t1,0);

  t = new Thread("t5_t2");
  t->Fork((VoidFunctionPtr)t5_t2,0);

}
#endif


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
  int num;

  for (num = 0; num < 5; num++) {
   printf("*** thread %d looped %d times\n", which, num);
   currentThread->Yield();
 }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
  DEBUG('t', "Entering SimpleTest");

  Thread *t = new Thread("forked thread");

  t->Fork(SimpleThread, 1);
  SimpleThread(0);
  DEBUG('t', "Entering TestSuite");
  TestSuite();
}
//Simple test cases for the threads assignment.

#ifdef CHANGED

//Global variables
Lock* recLineLock = new Lock("recLineLock");
int recLineCount[5] = {0,0,0,0,0};
Condition* recLineCV[5];
int recState[5] = {1,1,1,1,1}; //0 available, 1 busy, 2 on-break

int recTokens[5] = {0,0,0,0,0};
Lock* recLock[5];
Condition* recCV[5];
Lock* tokenLock = new Lock("tokenLock");
int nextToken=0;
int recCount = 5;

//Doctor globals
Lock* docLineLock = new Lock("docLineLock");
Condition* docLineCV;
Lock* docLock[5];
Condition* docCV[5];
int docState[5] = {1,1,1,1,1}; //0 available, 1 busy, 2 on-break

//Doorboy globals
Lock* docReadyLock = new Lock("docReadyLock");
bool docReady = false;

//Clerk globals
int consultingFee[5] = {0,0,0,0,0};
Condition* consultingCV[5];

/* Hospital members*/
void
Patient(int index){
  printf("Patient %d has arrived at the hospital. \n",index);
  recLineLock->Acquire();

	//Find shortest line or receptionist

  printf("Patient %d is looking for the best receptionist line to enter. \n",index);
	int shortest = recLineCount[0]; //Shortest line length
	int lineIndex = 0; //Index of line
	for(int i=1; i<recCount; i++){ //Go through each receptionist
		if(recLineCount[i] < shortest){ //If the next receptionist has a shorter line
			lineIndex = i; //Set index to this receptionist
			shortest = recLineCount[i]; //Set shortest line length to this one's
		}
		if(recState[i] == 0){ //If receptionist is open
			recState[i]=1; //Set receptionist's state to busy
			lineIndex = i; //Change line index to this receptionist
			break;
		}
	}
	
	if(shortest > -1){ //All Receptionists are busy, wait in line
		recLineCount[lineIndex]++; //Increment shortest line length
		recLineCV[lineIndex]->Wait(recLock[lineIndex]); //Wait till called
		recLineCount[lineIndex]--; //Decrement after being woken
	}

	recLineLock->Release(); //Release lock on line
	recLock[lineIndex]->Acquire(); //Acquire lock to receptionist
	recCV[lineIndex]->Signal(recLock[lineIndex]); //Notify receptionist ready
	recCV[lineIndex]->Wait(recLock[lineIndex]); //Wait for receptionist to reply
	int myToken = recTokens[lineIndex]; //Take token from receptionist
	recCV[lineIndex]->Signal(recLock[lineIndex]); //Notify receptionist token taken
  recLock[lineIndex]->Release(); //Release lock to receptionist

}

void
Receptionist(int index){
	while(true){
	  printf("Receptionist %d is about to acquire the lock.",index);
		recLineLock->Acquire(); //Acquire line lock
    printf("Receptionist %d has arrived at the hospital.",index);
		recState[index]=0; //Set self to not busy
		if(recLineCount[index] > 0){ //Check to see if anyone in line
			recLineCV[index]->Signal(recLock[index]); //Signal first person in line
			recState[index]=1; //Set self to busy
		}
		recLock[index]->Acquire(); //Acquire receptionist lock
		recLineLock->Release(); //Release line lock

		recCV[index]->Wait(recLock[index]); //Wait for patient to arrive
		tokenLock->Acquire(); //Acquire token lock
		recTokens[index]=nextToken; //Provide token to patient
		nextToken++; //Increment token count
		tokenLock->Release(); //Release token lock

		recCV[index]->Signal(recLock[index]); //Signal patient that token ready
		recCV[index]->Wait(recLock[index]); //Wait for patient to take token
		recLock[index] -> Release(); //Release lock on receptionist
	} //End of while
}

void
Doctor(int index){
  while(true){
  	int yieldCount = rand()%11+10; //Generate yield times between 10 and 20
  	for(int i = 0; i < yieldCount; i++){ //Check patient for that long
  		currentThread->Yield();
  	}
  	int sickTest = rand()%4; //Generate if patient is sick
  	if(sickTest == 0){ //Not sick

  	} else{ //Sick
  		switch(sickTest){
  			case 1:

        break;
        case 2:

        break;
        case 3:

        break;
        default:
        printf("Doctor: invalid sickness generated.");
      }
    }
  }
}

void
Cashier(){

}

void 
Door_Boy(){
  while(true){

  }
}

void
Clerk(){

}

void
Manager(){

}




//Tests and test threads for part 2 of the first assignment
void
Problem2()
{
  Thread *t;
  printf("Problem 2 start\n");

  t = new Thread("Receptionist 0");
  t->Fork(Patient,0);

  t = new Thread("Patient 0");
  t->Fork(Receptionist,0);

}


#endif

