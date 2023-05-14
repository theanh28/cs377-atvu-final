#include <ledger_queue.h>
#include <ledger.h>

/**
 * @brief Construct a new LedgerQueue object.
 */
LedgerQueue::LedgerQueue() {
  lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init(&lock, NULL);
  // Initially, queue is empty, so no subscribing worker should try to consume Ledgers.
  sem_init(&sem, 0, 0);
}

/**
 * @brief Destroy the LedgerQueue object
 * 
 * Destroys lock and semaphore.
 */
LedgerQueue::~LedgerQueue() {
  pthread_mutex_destroy(&lock);
  sem_destroy(&sem);
}

/**
 * @brief Enqueue a ledger and signal workers to consume.
 */
void LedgerQueue::push(Ledger ledger) {
  pthread_mutex_lock(&lock);
  queue.push_back(ledger);
  sem_post(&sem); // signal workers to consume.
  pthread_mutex_unlock(&lock);
}

/**
 * @brief Pop and return a Ledger to be consumed by queue.
 */
Ledger LedgerQueue::pop() {
  pthread_mutex_lock(&lock);
  // move syntax: relieve copying Ledger from queue to return value. Extra credit if possible pls :(
  Ledger ledger = std::move(queue.front());
  queue.pop_front();
  pthread_mutex_unlock(&lock);

  return ledger;
}

//////// LEDGER_QUEUE_MANAGER

/**
 * @brief Construct a new LedgerQueue object.
 */
LedgerQueueManager::LedgerQueueManager(int N) {
  lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init(&lock, NULL);

  this->queues = new LedgerQueue[N];
}

/**
 * @brief Destroy the LedgerQueue object
 * 
 * Destroys lock and semaphore.
 */
LedgerQueueManager::~LedgerQueueManager() {
  pthread_mutex_destroy(&lock);
  delete[] queues;
}

/**
 * @brief Enqueue a Ledger to its queue.
 */
void LedgerQueueManager::push(Ledger ledger) {
  // the `from` field of a Ledger stands for the acc number that 
  // a queue manages, and equal to the queue index. 
  queues[ledger.from].push(ledger);
}
