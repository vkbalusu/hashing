#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "hashes.hpp"

using namespace std;
using namespace hashes;

const uint32_t SEED{0};

void print_usage() {
  cout << "usage:" << endl
       << "    benchmark <STRUCTURE> <N>" << endl
       << endl
       << "where" << endl
       << "    <STRUCTURE> is one of: naive chain lp cuckoo" << endl
       << "    <N>: input size (positive integer)" << endl
       << endl;
}

int main(int argc, char* argv[]) {

  // parse commandline arguments

  vector<string> arguments(argv, argv + argc);

  if (arguments.size() != 3) {
    print_usage();
    return 1;
  }

  auto& structure = arguments[1],
        n_string = arguments[2];

  unsigned n;
  try {
    int parsed{stoi(n_string)};
    if (parsed <= 0) {
      cout << "error: input size " << parsed << " must be positive" << endl;
      return 1;
    }
    n = parsed;
  } catch (std::invalid_argument e) {
    cout << "error: '" << n_string << "' is not an integer" << endl;
    return 1;
  }
  assert(n > 0);

  srand(time(0));

  unique_ptr<abstract_dict<uint32_t>> dict;
  if (structure == "naive") {
    dict.reset(new naive_dict<uint32_t>(n));
  } else if (structure == "chain") {
    dict.reset(new chain_dict<uint32_t>(n));
  } else if (structure == "lp") {
    dict.reset(new lp_dict<uint32_t>(n));
  } else if (structure == "cuckoo") {
    dict.reset(new cuckoo_dict<uint32_t>(n));
  } else {
    print_usage();
    return 1;
  }
  assert(dict);

  // print parameters
  cout << "== dictionary benchmark ==" << endl
       << "structure: " << structure << endl
       << "n: " << n << endl;


  cout << "generating input..." << flush;
  vector<uint32_t> first_half,   // n/2 elements to insert
                   second_half,  // remaining n/2 elements to insert
                   absent; // n/2 elements to search for, that were never inserted
  {
    const unsigned half_n = n / 2,
      total_n = half_n * 3;

    // Create a random permutation of [0, total_n). This guarantees all
    // values are distinct.
    mt19937 gen(SEED);
    std::vector<uint32_t> randoms(total_n);
    for (unsigned i = 0; i < total_n; ++i) {
      randoms[i] = i;
    }
    std::shuffle(randoms.begin(), randoms.end(), gen);

    // Divide up the elements
    first_half.assign(randoms.begin(), randoms.begin() + half_n);
    second_half.assign(randoms.begin() + half_n, randoms.begin() + half_n * 2);
    absent.assign(randoms.begin() + half_n * 2, randoms.end());
  }

  auto check_all_present = [&](const vector<uint32_t>& vec) {
    for (auto x : vec) {
      try {
	auto& searched_value = dict->search(x);
	uint32_t expected_value = x + 1;
	if (searched_value != expected_value) {
	  cout << "error: search(" << x << ") found value " << searched_value
	       << ", which should be " << expected_value << endl;
	  return true;
	}
      } catch (std::out_of_range e) {
	cout << "error: search(" << x << ") failed";
	return true;
      }
    }
    return false;
  };

  auto check_all_absent = [&](const vector<uint32_t>& vec) {
    for (auto x : vec) {
      try {
	auto& searched_value = dict->search(x);
	cout << "error: search(" << x << ") found value " << searched_value
	     << ", but that key shouldn't be present" << endl;
	return true;
      } catch (std::out_of_range e) {
	// do nothing, expected to throw
      }
    }
    return false;
  };
  
  cout << endl << "inserting and searching for " << n << " elements..." << flush;

  // start high resolution clock
  using clock = chrono::high_resolution_clock;
  auto start = clock::now();

  // all elements should be absent
  if (check_all_absent(first_half)) {
    return 1;
  }
  if (check_all_absent(second_half)) {
    return 1;
  }
  if (check_all_absent(absent)) {
    return 1;
  }
  
  // insert first_half
  for (auto x : first_half) {
    dict->set(x, x + 1);
  }

  // only first_half should be present
  if (check_all_present(first_half)) {
    return 1;
  }
  if (check_all_absent(second_half)) {
    return 1;
  }
  if (check_all_absent(absent)) {
    return 1;
  }
  
  // insert second half
  for (auto x : second_half) {
    dict->set(x, x + 1);
  }

  // only first_half and second_half should be present
  if (check_all_present(first_half)) {
    return 1;
  }
  if (check_all_present(second_half)) {
    return 1;
  }
  if (check_all_absent(absent)) {
    return 1;
  }

  // stop the clock
  auto end = clock::now();

  // print elapsed time
  double seconds = chrono::duration_cast<chrono::duration<double>>(end - start).count();
  cout << endl << "elapsed time: " << seconds << " seconds" << endl;

  return 0;
}
