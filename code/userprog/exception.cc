 // exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include "addrspace.h"
#include <stdio.h>
#include <iostream>


using namespace std;
int MAX_CVS = 250;
int MAX_LOCKS = 250;

struct KernelLock{
    Lock* lock; 
    AddrSpace* as; 
    bool isToBeDestroyed; 
}; 
KernelLock kLocks[MAX_LOCKS]; //Set MAX_LOCKS to what you need 
int nextLockIndex = 0; 

Lock* lockTableLock = new Lock("lockTableLock"); 




int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
	bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
    	result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
      {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
   		}	

   		buf[n++] = *paddr;

   		if ( !result ) {
	//translation failed
   			return -1;
   		}

   		vaddr++;
   	}

   	delete paddr;
   	return len;
   }

   int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
   	bool result; 
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
    	result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

    	if ( !result ) {
	//translation failed
    		return -1;
    	}

    	vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
    	printf("%s","Bad pointer passed to Create\n");
    	delete buf;
    	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
    	printf("%s","Can't allocate kernel buffer in Open\n");
    	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
    	printf("%s","Bad pointer passed to Open\n");
    	delete[] buf;
    	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
    	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
    		delete f;
    	return id;
    }
    else
    	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.

    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
    	printf("%s","Error allocating kernel buffer for write!\n");
    	return;
    } else {
    	if ( copyin(vaddr,len,buf) == -1 ) {
    		printf("%s","Bad pointer passed to to write: data not written\n");
    		delete[] buf;
    		return;
    	}
    }

    if ( id == ConsoleOutput) {
    	for (int ii=0; ii<len; ii++) {
    		printf("%c",buf[ii]);
    	}

    } else {
    	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
    		f->Write(buf, len);
    	} else {
    		printf("%s","Bad OpenFileId passed to Write\n");
    		len = -1;
    	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
    	printf("%s","Error allocating kernel buffer in Read\n");
    	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
    	scanf("%s", buf);

    	if ( copyout(vaddr, len, buf) == -1 ) {
    		printf("%s","Bad pointer passed to Read: data not copied\n");
    	}
    } else {
    	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
    		len = f->Read(buf, len);
    		if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
    			if ( copyout(vaddr, len, buf) == -1 ) {
    				printf("%s","Bad pointer passed to Read: data not copied\n");
    			}
    		}
    	} else {
    		printf("%s","Bad OpenFileId passed to Read\n");
    		len = -1;
    	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
	OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

	if ( f ) {
		delete f;
	} else {
		printf("%s","Tried to close an unopen file\n");
	}
}

#ifdef CHANGED
struct ThreadEntry { //Struct to represent a thread
    int firstStackPage;
    Thread* myThread;
    ThreadEntry() : firstStackPage(-1), myThread(NULL) {}
}; 

struct ProcessEntry { //Struct to represent a process
    int threadCount;
    int spaceID;
    AddrSpace* as;
    ThreadEntry threads[100];
};

ProcessEntry processTable[10] = {};
Lock* processTableLock = new Lock("processTableLock");
int spaceIDCount = 1;

ThreadEntry createThreadEntry(Thread* th, int stackPage){
    ThreadEntry* te = new ThreadEntry();
    te->firstStackPage = stackPage;
    te->myThread = th;
    return *te;
}

ProcessEntry createProcessEntry(Thread* th, AddrSpace* addrs){
    ThreadEntry thread1 = createThreadEntry(th, 0); //Create the first threadentry for process

    ProcessEntry entry = {}; //Create process entry
    entry.threadCount = 0;
    entry.as = addrs;
    entry.threads[0] = thread1;
    return entry;
}

struct forkInfo{
	int vaddr;
	int pageLoc;
};

void fork_thread(int value){
	forkInfo *m = (forkInfo*) value;
	int vaddr = m->vaddr;
	int stackStart = m->pageLoc;

	machine->WriteRegister(PCReg, vaddr);
	machine->WriteRegister(NextPCReg, vaddr+4);

	//Write to stack register the starting point of the stack for this thread
	machine->WriteRegister(StackReg, 0 ); //numPages * PageSize - 16

	currentThread->space->RestoreState();

	machine->Run();
}

void Fork_Syscall(unsigned int vaddr){
	Thread* t = new Thread("forkThread"); //Create new thread
	t->space = currentThread->space; //Allocate the addrspace to the thread being forked, same as current thread's

	forkInfo tmp;
	tmp.vaddr = vaddr;

	//Find 8 pages of stack to give to thread?

	tmp.pageLoc = 0;

	//Multiprogramming: Update process table
	ThreadEntry te = createThreadEntry(t, 0); //Give first stack page?
	processTableLock->Acquire();
	for(int i = 0; i < 10; i++){
		if(processTable[i].as == currentThread->space){ //What value is empty brackets initialized to?
			processTable[i].threads[processTable[i].threadCount] = te;
			tmp.pageLoc = processTable[i].threadCount;
			processTable[i].threadCount++;
			break;
		}
	}
	processTableLock->Release();


	t->Fork(fork_thread, (int) &tmp);
}

struct execInfo{

};

void exec_thread(int value){
	//Initialize the register by using currentThread->space.

	currentThread->space->RestoreState();
	machine->Run();
}

void Exec_Syscall(unsigned int vaddr, char *filename){
	//Convert VA to physical address
	int addr = vaddr; 

	OpenFile* f = fileSystem->Open(filename);
	if (f == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
    }

	AddrSpace *space; // Create new addresspace for this executable file.
    space = new AddrSpace(f);

	// Store its openfile pointer?
	space->fileTable.Put(f);

	Thread *t = new Thread(""); //Create a new thread
	t->space = space; // Allocate the space created to this thread's space.

	// Update the process table and related data structures.

	// Write the space ID to the register 2.
	machine->WriteRegister(2, 0); 

	execInfo tmp;
	t->Fork(exec_thread, (int) &tmp);
}

int Exit_Syscall(){

	currentThread->Finish();
	return 0;
}

void Yield_Syscall(){
	currentThread->Yield();
}



int CreateLock_Syscall(char* debugName){
	KernelLock* tempLock = new KernelLock;
	tempLock->as = currentThread->space;
	tempLock->isToBeDestroyed = false;
	tempLock->lock = new Lock(debugName);
	lockTableLock->Acquire();
	if (nextLockIndex < MAX_LOCKS){
		kLocks[nextLockIndex] = tempLock;
		nextLockIndex++;
	}
	lockTableLock->Release();
	return nextLockIndex-1;
}

void DestroyLock_Syscall(int index){
	kLocks[index].isToBeDestroyed = true;
	if (kLocks[index].isToBeDestroyed && kLocks[index].lock->isFree){
		delete kLocks[index].lock;
		delete kLocks[index];
		kLocks[index] = NULL;
	}
	return;
}

void Acquire_Syscall(int index){
	kLocks[index]->lock->Acquire();
	return;
}

void Release_Syscall(int index){
	kLocks[index]->lock->Release();
	if (kLocks[index]->lock->isFree && kLocks[index]->isToBeDestroyed){
		DestroyLock_Syscall(index);
	}
	return;
}

struct KernelCV{ 
	Condition* condition; 
	AddrSpace* as; 
	bool isToBeDestroyed; 
};
KernelCV kCV[MAX_CVS]; //Set MAX_LOCKS to what you need 
int nextCVIndex = 0; 

Lock* CVTableLock = new Lock("CVTableLock"); 


int CreateCondition_Syscall(char* debugName){
	KernelCV* tempCV = new KernelCV;
	tempCV->as = currentThread->space;
	tempCV->isToBeDestroyed = false;
	tempCV->condition = new Lock(debugName);
	lockTableLock->Acquire();
	if (nextCVIndex < MAX_LOCKS){
		kCV[nextCVIndex] = tempCV;
		nextCVIndex++;
	}
	CVTableLock->Release();
	return nextCVIndex-1;
}

void DestroyCondition_Syscall(int index){
	kCV[index]->isToBeDestroyed = true;
	if (kCV[index]->isToBeDestroyed && kCV[index]->condition->waitingLock == NULL){
		delete kCV[index]->condition;
		delete kCV[index];
		kCV[index] = NULL;
	}
	return;
}

void Wait_Syscall(int index){
	kCV[index]->condition->Wait();
	return;
}

void Signal_Syscall(int index){
	kCV[index]->condition->Signal();
	if (kCV[index]->isToBeDestroyed && kCV[index]->condition->waitingLock == NULL){
		DestroyCondition_Syscall();
	}
	return;
}

void Broadcast_Syscall(int index){
	kCV[index]->condition->Broadcast();
	if (kCV[index]->isToBeDestroyed){
		DestroyCondition_Syscall();
	}
	return;
}

void 

bool validateAddress(unsigned int vaddr){
	if(vaddr == NULL)
		return false;
	if( vaddr > 0 && vaddr <  ){ //Check if vaddr is within bounds
		return true;
	}
	return false;
}
#endif

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
    	switch (type) {
    		default:
    		DEBUG('a', "Unknown syscall - shutting down.\n");
    		case SC_Halt:
    		DEBUG('a', "Shutdown, initiated by user program.\n");
    		interrupt->Halt();
    		break;
    		case SC_Create:
    		DEBUG('a', "Create syscall.\n");
    		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
    		break;
    		case SC_Open:
    		DEBUG('a', "Open syscall.\n");
    		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
    		break;
    		case SC_Write:
    		DEBUG('a', "Write syscall.\n");
    		Write_Syscall(machine->ReadRegister(4),
    			machine->ReadRegister(5),
    			machine->ReadRegister(6));
    		break;
    		case SC_Read:
    		DEBUG('a', "Read syscall.\n");
    		rv = Read_Syscall(machine->ReadRegister(4),
    			machine->ReadRegister(5),
    			machine->ReadRegister(6));
    		break;
    		case SC_Close:
    		DEBUG('a', "Close syscall.\n");
    		Close_Syscall(machine->ReadRegister(4));
    		break;

		#ifdef CHANGED
    		case SC_Fork:
    		DEBUG('a', "Fork syscall.\n");
    		Fork_Syscall(machine->ReadRegister(4));
    		break;

    		case SC_Exec:
    		DEBUG('a', "Exec syscall.\n");
    		Exec_Syscall(machine->ReadRegister(4));
    		break;

    		case SC_Exit:
    		DEBUG('a', "Exit syscall.\n");
    		rv = Exit_Syscall(machine->ReadRegister(4));
    		break;

    		case SC_Yield:
    		DEBUG('a', "Yield syscall.\n");
    		Yield_Syscall(machine->ReadRegister(4));
    		break;

            case SC_CreateLock:
            DEBUG('a', "Create Lock syscall.\n");
            rv = CreateLock_Syscall(machine->ReadRegister(4));
            break;

            case SC_DestroyLock:
            DEBUG('a', "Destroy Lock syscall.\n");
            
            break;

            case SC_Acquire:
            DEBUG('a', "Lock Acquire syscall.\n");

            break;

            case SC_Release:
            DEBUG('a', "Lock Release syscall.\n");

            break;

            case SC_CreateCondition:
            DEBUG('a', "Create Condition syscall.\n");

            break;

            case SC_DestroyCondition:
            DEBUG('a', "Destroy Condition syscall.\n");

            break;

            case SC_Wait:
            DEBUG('a', "Wait syscall.\n");

            break;

            case SC_Signal:
            DEBUG('a', "Signal syscall.\n");

            break;

            case SC_Broadcast:
            DEBUG('a', "Broadcast syscall.\n");

            break;

	    #endif
        }

	// Put in the return value and increment the PC
        machine->WriteRegister(2,rv);
        machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
        return;
    } else {
    	cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
    	interrupt->Halt();
    }
}
