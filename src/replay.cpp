
#include <iostream>
#include <fstream>
#include <string>

#include "replay.h"
// 3 second delay at start

struct __attribute__((__packed__)) ReplayHeader {
    unsigned long num_ticks;
};

using namespace std;
static_assert(sizeof(ssize_t) == 8, "size_t should be 8");
Replay::Replay(const char *filename) {
	ifstream f(filename, ios::binary|ios::in);
	if (!f.is_open()) {
		std::cout << "Could not open " << filename << std::endl;
		exit(1);
		return;
	}
	struct ReplayHeader head;
	f.read(reinterpret_cast<char*>(&head), sizeof head);

	if (!f) {
		std::cout << "Invalid save file " << filename << std::endl;
		exit(1);
		return;
	}
	std::cout << "Loaded replay with " << head.num_ticks << "  ticks" << std::endl;

	ssize_t n;
	for (int i=0; i<head.num_ticks; i++) {
	        f.read(reinterpret_cast<char*>(&n), 8);

		if (!f) {
			std::cout << "Invalid save file " << filename << std::endl;
			exit(1);
			return;
		}
		replay.push_back(n);
		// printf("%f %f %ld\n", n.x, n.y, n.time_due);
	}


}
