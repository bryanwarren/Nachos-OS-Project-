#ifndef __RWLOCK_H__
#define __RWLOCK_H__
#include <pthread.h>
#include<semaphore.h>
#include "synch.h"

class RWLock{
private:
  int AR; //active readers
  int WR; //waiting readers
  int AW; //active writers
  int WW; //waiting readers
  Condition* okToRead;
  Condition* okToWrite;
  Lock* lock;

public:
    RWLock();
    ~RWLock();
    //Reader
    void startRead();
    void doneRead();
    // Writer
    void startWrite();
    void doneWrite();
};

#endif
