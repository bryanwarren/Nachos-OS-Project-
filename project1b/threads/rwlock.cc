#include<stdio.h>
#include <pthread.h>
#include <iostream>
#include "synch.h"
#include "rwlock.h"

RWLock::RWLock() {
  //initialize all variables
  this->AR = 0; 
  this->WR = 0; 
  this->AW = 0;
  this->WW = 0;
  char* debug = (char*) "d";
  lock = new Lock(debug);
  okToRead = new Condition(debug);
  okToWrite = new Condition(debug);
  
}
RWLock::~RWLock() {
  delete lock;
  delete okToRead;
  delete okToWrite;
  //destroy conditions and lock
}

void RWLock::startRead() {
  lock->Acquire();
  while((this->AW+this->WW)>0){
    this->WR++;
    okToRead->Wait(lock);
    this->WR--;
  }
  this->AR++;
  lock->Release();

}
void RWLock::doneRead() {
  lock->Acquire();
  this->AR--;
  if(this->AR==0 && this->WW > 0)
    okToWrite->Signal(lock);
  lock->Release();
}
void RWLock::startWrite() {
  lock->Acquire();
  while((this->AW+this->AR)>0){
    this->WW++;
    okToWrite->Wait(lock);
    this->WW--;
  }
  this->AW++;
  lock->Release();
  
}
void RWLock::doneWrite() {
  lock->Acquire();
  this->AW--;
  if(this->WW>0)
    okToWrite->Signal(lock);
  else if(this->WR>0)
    okToRead->Broadcast(lock);
  lock->Release();
}
