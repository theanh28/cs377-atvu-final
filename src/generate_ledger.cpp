#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <ledger_file_name> <num_of_ledgers>\n" << endl;
    exit(-1);
  }

  srand(std::time(NULL));

  char* ledger_file_name = argv[1];
  int num_ledgers = atoi(argv[2]);

  ofstream outfile(ledger_file_name);
  for (int i = 0; i < num_ledgers; ++i) {
    outfile << rand() % 10 << ' ' << rand() % 10 << ' ' << rand() % 300 << ' ' << rand() % 3 << '\n';
  }
  outfile.close();

  return 0;
}
