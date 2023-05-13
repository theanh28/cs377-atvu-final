#include <ledger.h>

int main(int argc, char* argv[]) {
  if (argc != 4) {
    cerr << "Usage: " << argv[0] << " <num_of_readers> <num_of_threads_per_bank_account> <ledger_file>\n" << endl;
    exit(-1);
  }

  int num_readers = atoi(argv[1]);
  int num_workers_per = atoi(argv[2]);
  StartBank(num_readers, num_workers_per, argv[3]);

  return 0;
}
