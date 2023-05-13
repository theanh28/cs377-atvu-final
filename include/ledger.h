#ifndef _LEDGER_H
#define _LEDGER_H

#include <bank.h>

using namespace std;

#define D 0
#define W 1
#define T 2

const int SEED_RANDOM = 377;

struct Ledger {
	int from;
	int to;
	int amount;
	int mode;
	int ledgerID;
	// each Ledger will be retried at least 2 times.
	int retry = 2;
};

// Used to pass args into worker threads.
struct WorkerThreadArgs {
	// contrary to id given to threads, this id is defined by us
	// as the queue_id/bank acc number that  the worker works with.
	// Used to identify Ledger queue and log for human readers.
	int queue_id;
	// identify worker in case we use multiple worker for a queue.
	// Used to log for human readers.
	int sub_id; 
};

extern list<struct Ledger> ledger;

void StartBank(int num_readers, int num_workers_per_acc, char *filename);
void *reader(void *arg);
void *worker(void *args);
void *idle_check(void *arg);

#endif