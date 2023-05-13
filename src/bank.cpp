#include <bank.h>

/**
 * @brief prints account information
 */
void Bank::print_account() {
  for (int i = 0; i < num; i++) {
    pthread_mutex_lock(&accounts[i].lock);
    cout << "ID# " << accounts[i].accountID << " | " << accounts[i].balance
         << endl;
    pthread_mutex_unlock(&accounts[i].lock);
  }

  pthread_mutex_lock(log_lock);
  cout << "Success: " << num_succ << " Fails: " << num_fail << endl;
  pthread_mutex_unlock(log_lock);
}

/**
 * @brief helper function to increment the bank variable `num_fail` and log 
 *        message.
 * 
 * @param message
 */
void Bank::recordFail(char *message) {
  pthread_mutex_lock(log_lock);
  cout << message << endl;
  num_fail++;
  pthread_mutex_unlock(log_lock);
}

/**
 * @brief helper function to increment the bank variable `num_succ` and log 
 *        message.
 * 
 * @param message
 */
void Bank::recordSucc(char *message) {
  pthread_mutex_lock(log_lock);
  cout << message << endl;
  num_succ++;
  pthread_mutex_unlock(log_lock);
}

/**
 * @brief Construct a new Bank:: Bank object.
 * 
 * Requirements:
 *  - The function should initialize the private variables.
 *  - Create a new array[N] of type Accounts. 
 *  - Initialize each account (HINT: there are three fields to initialize)  
 * 
 * @param N 
 */
Bank::Bank(int N, pthread_mutex_t *log_lock) {
  this->log_lock = log_lock;
  num = N;
  accounts = new Account[num];
  num_succ = 0;
  num_fail = 0;
  for (int i = 0; i < num; ++i) {
    accounts[i].accountID = i;
    accounts[i].balance = 0;
    accounts[i].lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&accounts[i].lock, NULL); // hopefully no error here.
  }
}

/**
 * @brief Destroy the Bank:: Bank object
 * 
 * Requirements:
 *  - Make sure to destroy all locks. 
 *  - Make sure to free all memory
 * 
 */
Bank::~Bank() {
  for (int i = 0; i < num; ++i) {
    pthread_mutex_destroy(&accounts[i].lock);
  }
  delete[] accounts;
}

/**
 * @brief Adds money to an account 
 * 
 * Requirements:
 *  - Make sure to log in the following format
 *    `Worker [worker_id] completed ledger [ledger_id]: deposit [amount] into account [account]`
 * 
 * @param workerID the ID of the worker (thread)
 * @param ledgerID the ID of the ledger entry
 * @param accountID the account ID to deposit 
 * @param amount the amount deposited
 * @return int 
 */
int Bank::deposit(char* workerID, int ledgerID, int accountID, int amount) {
  char msg[100];
  pthread_mutex_lock(&accounts[accountID].lock);
  accounts[accountID].balance += amount;
  sprintf(msg, "Worker %s completed ledger %d: deposit %d into account %d", workerID, ledgerID, amount, accountID);
  recordSucc(msg);
  pthread_mutex_unlock(&accounts[accountID].lock);
  return 0;
}

/**
 * @brief Withdraws money from an account
 * 
 * Requirements:
 *  - Make sure the account has a large enough balance. 
 *    - Case 1: withdraw amount <= balance, log success 
 *    - Case 2: log failure
 * 
 * @param workerID the ID of the worker (thread)
 * @param ledgerID the ID of the ledger entry
 * @param accountID the account ID to withdraw 
 * @param amount the amount withdrawn
 * @return int 0 on success -1 on failure
 */
int Bank::withdraw(char* workerID, int ledgerID, int accountID, int amount) {
  char msg[100];
  pthread_mutex_lock(&accounts[accountID].lock);
  if (accounts[accountID].balance >= amount) {
    accounts[accountID].balance -= amount;
    sprintf(msg, "Worker %s completed ledger %d: withdraw %d from account %d", workerID, ledgerID, amount, accountID);
    recordSucc(msg);
    pthread_mutex_unlock(&accounts[accountID].lock);
  } else {
    sprintf(msg, "Worker %s failed to complete ledger %d: withdraw %d from account %d", workerID, ledgerID, amount, accountID);
    recordFail(msg);
    pthread_mutex_unlock(&accounts[accountID].lock);
    return -1;
  }
  return 0;
}

/**
 * @brief Transfer from one account to another
 * 
 * Requirements:
 *  - Make sure there is enough money in the FROM account
 *  - Be careful with the locking order
 *  
 * 
 * @param workerID the ID of the worker (thread)
 * @param ledgerID the ID of the ledger entry
 * @param srcID the account to transfer money out 
 * @param destID the account to receive the money
 * @param amount the amount to transfer
 * @return int 0 on success -1 on error
 */
int Bank::transfer(char* workerID, int ledgerID, int srcID, int destID,
                   unsigned int amount) {
  char msg[100];
  if (srcID == destID) {
    pthread_mutex_lock(&accounts[srcID].lock);
    sprintf(msg, "Worker %s failed to complete ledger %d: transfer %d from account %d to account %d", workerID, ledgerID, amount, srcID, destID);
    recordFail(msg);
    pthread_mutex_unlock(&accounts[srcID].lock);
    return -1;
  }
  pthread_mutex_lock(&accounts[srcID].lock);
  pthread_mutex_lock(&accounts[destID].lock);
  if (accounts[srcID].balance >= amount) {
    accounts[srcID].balance -= amount;
    accounts[destID].balance += amount;

    sprintf(msg, "Worker %s completed ledger %d: transfer %d from account %d to account %d", workerID, ledgerID, amount, srcID, destID);
    recordSucc(msg);
    pthread_mutex_unlock(&accounts[destID].lock);
    pthread_mutex_unlock(&accounts[srcID].lock);
  } else {
    sprintf(msg, "Worker %s failed to complete ledger %d: transfer %d from account %d to account %d", workerID, ledgerID, amount, srcID, destID);
    recordFail(msg);
    pthread_mutex_unlock(&accounts[destID].lock);
    pthread_mutex_unlock(&accounts[srcID].lock);
    return -1;
  }
  return 0;
}