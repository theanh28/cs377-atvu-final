# cs377-atvu-final
C++ multi-threading (concurrent) message queue for bank transactions.

## Slides and video
Here's the [slides](https://docs.google.com/presentation/d/1YfZQOTx1mCIu35VXGmmKUYVZ3bcSaAhIkxsK7tuhOXc/edit?usp=sharing) and a [video](#) of me reading it.

## Goals and solution
The original project 4, empowered with message queues brought to you by semaphores. We strive to achieve 3 goals: 
1. Multi concurrent ledger readers and workers. 
    
    In the original project 4, we use a main thread to parse all ledgers from an input file into a queue, then spin multiple worker threads to concurrently take those ledgers and perform transactions. 
    
    In a real world scenario, ledgers input are not contained in a definite file but are made continuously, and need to be processed as they appear. Also, for scalability, it is pivotal to use multiple readers to read ledgers and multiple workers to process them. We implement this by using a semaphore for each queue. Initially a semaphore is 0. We will have multiple reader threads parsing and pushing ledgers into a queue, which will call `sem_post()` and  increase the semaphore. Workers that consume this queue will use `sem_wait()` on the semaphore, and get woken up once a new ledger appear.

2. Dedicated queue for each account.
    
    Inspired by kafka topic, we would like to create a queue for each bank account,  and have ledgers of this account to be processed by designated workers. In this project, each bank account has the same number of designated workers, but it can be expanded into having different number of workers, reflecting real world application of customer plans. We achieve this goal of 1 queue per account by... creating a queue for every account, and have designated workers subscribe to that queue. A worker subscribe to a queue by calling `sem_wait()` on its semaphore.

3. Ledger Retry.

    It is common for ledgers, namely Transfer ledger, to fail due to source account not having enough money. Therefore we implement a retry mechanism for each ledger to increase the chance of success. For each ledger, we add a `retry` field that gets decremented whenever a worker fails to perform the ledger. This worker then add the ledger back to its queue if there are still retries.


## Design

> The slides present this design with much better illustrations, PLEASE CHECK THE DESIGN SECTION OF THE [SLIDES](https://docs.google.com/presentation/d/1YfZQOTx1mCIu35VXGmmKUYVZ3bcSaAhIkxsK7tuhOXc/edit?usp=sharing). There are presenter notes explaining the slides below each slide.**

Also there are code comments for each and every APIs and fields in the project that should help with understanding.
### Overview
The design section aims to give an overview of each component of the project. Further details can be found in code comments (and the code itself is clean). The project consists of 5 main classes, and 3 thread functions. While 3 classes, Ledger, Account, and Bank, are from the original project 4, we also introduce a new LedgerQueue and LedgerQueueManager class, and modify the Ledger class with retry.

### LedgerQueue
For each bank account, we instantiate a LedgerQueue object that holds ledgers interacting with that bank account. We recall from project 4, that each ledger contains a field `from` denoting the account that money is taken from or added to. As a result, we add ledgers to LedgerQueue based on their `from` field. Workers will subscribe to a LedgerQueue in order to consume ledgers from the queue. LedgerQueue class holds a mutex `lock` to control race condition from multiple workers accessing queue, and a semaphore `sem` to signal workers upon new ledgers being added to the queue. The class exposes a .push() and .pop() API to add and extract ledgers from the queue.

### LedgerQueueManager
The LedgerQueueManager essentially instantiates and hold the LedgerQueues. It also exposes a .push() API that receives a ledger and move it to the corresponding LedgerQueue using the ledger's `from` field.

### Thread functions
We continue with the 3 thread functions. Firstly, reader threads parse ledgers from the input file and add it to LedgerQueue through LedgerQueueManager. Secondly, worker threads consume queues of their designated bank account, perform the ledger, retry if fail to perform ledger by reducing a retry count, and wait for new ledgers using `sem_wait()` on the semaphore of the queue. Remember that a ledger being added to a LedgerQueue will invoke `sem_post()`, which increases the semaphore and wake up worker waiting with `sem_wait()`. Also, every time the worker process a ledger, it set a global variable `last_ledger_processed`, which is used by the idle check thread function. This function checks the program if all worker threads stopped processing ledgers for at least 5 seconds by comparing `last_ledger_processed` global variable with current time. A time difference of more than 5 seconds implies there are no ither ledgers to be parsed or processed, aka reader threads reaching the end of input file. We stop the program in this case.

### Sum up
To sum up, the main thread prepares a Bank object and LedgerQueueManager objects, which instantiates Accounts and a LedgerQueue for each. The main thread then spins up multiple reader threads to parse ledgers from the input file and add to LedgerQueues, and workers threads that consume those queues. A ledger being parsed and added to a LedgerQueue will wake up a worker. The worker then extracts the ledger from the LedgerQueue, perform the ledger, push back to queue in case of retry, and reset the `last_queue_processed` varaible. The variable is checked by an idle check thread.
