#include <unistd.h>

#include <ledger.h>
#include <ledger_queue.h>
#include <time.h>

using namespace std;

// GLOBAL VARIABLES
Bank *bank;
LedgerQueueManager *LQManager;
ifstream input_file;
int next_ledger_id = 0; // shared by readers.
const int bank_acc = 10; // number of bank accounts.

// GLOBAL LOCKS
pthread_mutex_t reader_lock; // control concurrent ledger reading.
pthread_mutex_t log_lock; // control different sources of concurrent logs (e.g. parse ledger log, transaction log).

// STOP MECHANISM.
bool completed = false; // boolean used to stop the workers after all ledgers have been processed.
time_t last_ledger_processed = NULL; // used to stop workers if last ledger was processed > 5s ago, aka idle time > 5s.

/**
 * @brief Init and start bank, queues, and threads.
 *  
 * @param num_readers - number of threads that read Ledger from input file. 
 * @param num_workers_per_queue - number of worker threads for each bank acct. Since
 * 																each bank acct correspond to a Ledger queue, this
 *  															is also the number of worker per queue.
 * @param filename - input file name.
 */
void StartBank(int num_readers, int num_workers_per_acc, char *filename) {
	log_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&log_lock, NULL);

	bank = new Bank(bank_acc, &log_lock);
	bank->print_account();

	// Instantiate 10 Ledger queues corresponding to 10 accounts.
	LQManager = new LedgerQueueManager(bank_acc);

	// Open input file.
	input_file.open(filename);
	if (!input_file) {
		std::cerr << "Failed to open file " << filename << '\n';
		return; 
	}

	// Start readers.
	pthread_mutex_init(&reader_lock, NULL);
	pthread_t readers[num_readers];
	int reader_args[num_readers];
	for (int i = 0; i < num_readers; ++i) {
		reader_args[i] = i;
		pthread_create(&readers[i], NULL, reader, &reader_args[i]);
	}
	
	// Start workers
	int total_worker = num_workers_per_acc * bank_acc;
	pthread_t workers[total_worker];
	struct WorkerThreadArgs thread_args[total_worker];
	for (int i = 0; i < bank_acc; ++i) {
		// For each queue, start `num_workers_per_acc` workers consuming that queue.
		for (int j = 0; j < num_workers_per_acc; ++j) {
			int worker_idx = i * num_workers_per_acc + j;
			thread_args[worker_idx].queue_id = i;
			thread_args[worker_idx].sub_id = j;
			pthread_create(&workers[worker_idx], NULL, worker, 
											&thread_args[worker_idx]);
		}
	}

	// Spins idle checking thread to stop program after 5s idle.
	pthread_t idle_check_thread;
	int idle_check_thread_arg = num_workers_per_acc;
	pthread_create(&idle_check_thread, NULL, idle_check, 
									&idle_check_thread_arg);

	// Joining threads.
	for (int i = 0; i < num_readers; ++i) {
		pthread_join(readers[i], NULL);
	}
	for (int i = 0; i < total_worker; ++i) {
		pthread_join(workers[i], NULL);
	}
	pthread_join(idle_check_thread, NULL);

	bank->print_account();

	input_file.close();
	pthread_mutex_destroy(&reader_lock);
	pthread_mutex_destroy(&log_lock);
	delete bank;
	delete LQManager;
}

void recordReaderParse(int reader_id, int ledger_id) {
	pthread_mutex_lock(&log_lock);
	cout << "Reader " << reader_id << " successfully parsed ledger " << ledger_id << '\n';
	pthread_mutex_unlock(&log_lock);
}

/**
 * @brief Parse the input ledger file and store ledger into respective queue.
 * 
 * This function parses the input file for a ledger, determine the bank account
 * that the ledger corresponds to, and push the ledger to the Ledger queue of
 * that bank account through `LQManager`.
 * 
 * @param arg - thread params holding reader_id used for logging.
 * @return void*
 */
void *reader(void* arg){
	int reader_id = *(int *)arg;
	int f, t, a, m;
	pthread_mutex_lock(&reader_lock);
	while (input_file >> f >> t >> a >> m) {
		int ledger_id = next_ledger_id ++;
		// unlock after assigning ledger ID and last read to prevent race condition.
		pthread_mutex_unlock(&reader_lock);

		struct Ledger l;
		l.from = f;
		l.to = t;
		l.amount = a;
		l.mode = m;
		l.ledgerID = ledger_id;
		LQManager->push(l);

		recordReaderParse(reader_id, l.ledgerID);

		pthread_mutex_lock(&reader_lock); // lock to read at next loop.
		// record reader finishes parsing a Ledger, and we utilize the lock too.
	}
	pthread_mutex_unlock(&reader_lock);
	return NULL;
}

/**
 * @brief Remove items from the list and execute the instruction.
 * 
 * @param args - {queue_id, sub_id} of the worker, used to determine the bank acct and
 * 								Ledger queue to consume, as well as logging.
 * @return void* 
 */
void* worker(void *args){
	struct WorkerThreadArgs* thread_args = (struct WorkerThreadArgs*) args;
	int queue_id = thread_args->queue_id;
	int sub_id = thread_args->sub_id;
	char worker_id[10];
	sprintf(worker_id, "%d.%d", queue_id, sub_id);

	// Ledger queue to consume ledger from, to perform transaction.
	LedgerQueue& queue = LQManager->queues[queue_id];

	// Subscribe to a Ledger queue by getting its semaphore, which is used
	// to detect signal when queue receives new ledger. 
	sem_t* queue_sem = &queue.sem;
	
	while (true) {
		// Wait for queue to signal new ledgers.
		sem_wait(queue_sem);
		pthread_mutex_lock(&log_lock);
		pthread_mutex_unlock(&log_lock);
		if (completed) { // program stop mechanism.
			return NULL;
		}
		last_ledger_processed = time(NULL); // reset idle time.
		Ledger ledger = queue.pop();
		switch (ledger.mode) {
			case D:
				bank->deposit(worker_id, ledger.ledgerID, ledger.from, ledger.amount);
				break;
			case W:
				bank->withdraw(worker_id, ledger.ledgerID, ledger.from, ledger.amount);
				break;
			case T:
				// Bank::transfer() is the only transaction that can fail. We retry the transaction upto `Ledger::retry`. 
				if (bank->transfer(worker_id, ledger.ledgerID, ledger.from, ledger.to, ledger.amount) < 0
						&& ledger.retry > 0) {
					-- ledger.retry;
					queue.push(ledger);
				}
				break;
			default: // unknown mode, skipping
				break;
		}
	}
	return NULL;
}

/**
 * @brief Check idling status of the program and stop program when idle for > 5s.
 * 
 * @param arg - void* pointing to `worker_per_acc` value. 
 * 
 * Similar to a spin lock, this function keeps spinning and check, every second, the `last_ledger_processed` 
 * for program idle time, which infers no other ledgers to parse or process. When program has been idle for
 * > 5s, this function sets `completed` to true to signal the end of the program, and wake up worker threads 
 * using the queue semaphores.
 * 
 * @return void* 
 */
void *idle_check(void* arg) {
	int worker_per_acc = *(int *) arg;
	while (true) {
		sleep(1);
		if (last_ledger_processed == NULL) {
			// default value implies no ledger has been processed.
			continue;
		}
		int time_diff = (int)(time(NULL) - last_ledger_processed);
		if (time_diff > 5) {
			break;
		}
	}

	// At this point, there should be no more logging from other threads, 
	// so we can cout without lock.
	cout << "---> ending program \n";
	completed = true;
	// Wake up each worker thread by signaling queue semaphore for 'worker_per_acc' times.
	for (int i = 0; i < bank_acc; ++i) {
		for (int j = 0; j < worker_per_acc; ++j) {
			// signal queue semaphore once for each worker thread consuming this queue.
			sem_post(&(LQManager->queues[i].sem));
			pthread_mutex_lock(&log_lock);
			cout << "--> Signal worker " << i << "." << j  << "to end.\n";
			pthread_mutex_unlock(&log_lock);
		}
	}
	return NULL;
}