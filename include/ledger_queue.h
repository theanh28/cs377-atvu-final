#ifndef _LEDGER_QUEUE_H
#define _LEDGER_QUEUE_H

#include <ledger.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <sys/wait.h>   /* for wait() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <sys/mman.h>   /* for mmap() ) */
#include <semaphore.h>  /* for sem */
#include <assert.h>		/* for assert */
#include <iostream>     /* for cout */
#include <list>
#include <array>
#include <pthread.h> 

using namespace std;

class LedgerQueue {
  private:
    pthread_mutex_t lock; // workers and LedgerQueueManager concurrently interact with LedgerQueue.
    list<struct Ledger> queue; // holding ledgers, which are produced by readers and consumed by workers. 
  public:
    LedgerQueue();
    ~LedgerQueue();

    void push(Ledger ledger);
    struct Ledger pop();

    sem_t sem; // used to signal workers when queue receives new Ledgers.
};

class LedgerQueueManager {
  private:
    pthread_mutex_t lock; // multiple readers interact with this manager.

  public:
    LedgerQueueManager(int N);
    ~LedgerQueueManager();

    void push(struct Ledger ledger); // moves ledger to corresponding LedgerQueue.

    LedgerQueue* queues;
};

#endif // _LEDGER_QUEUE_H