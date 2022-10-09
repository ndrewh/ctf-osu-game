#include <iostream>
#include <fstream>
#include <string>

#include "level_save.h"
#define NOTE_SIZE 24
#define CONSTRAINT_SIZE 64
// 3 second delay at start
struct __attribute__((__packed__)) NoteRaw {
    float x, y;
    unsigned long time_due;
    unsigned char r, g, b;
};

struct __attribute__((__packed__)) ConstraintRaw {
    unsigned long circ_idx[4];
    unsigned int code[8];
};

struct __attribute__((__packed__)) GameHeader {
    unsigned long num_notes, num_constraints;
};

using namespace std;
LevelSave::LevelSave(std::string &filename) {
	max_tick = 0;
	ifstream f(filename, ios::binary|ios::in);
	if (!f.is_open()) {
		std::cout << "Could not open " << filename << std::endl;
		exit(1);
		return;
	}
	struct GameHeader head;
	f.read(reinterpret_cast<char*>(&head), sizeof head);

	if (!f) {
		std::cout << "Invalid save file " << filename << std::endl;
		exit(1);
		return;
	}
	std::cout << "Loaded save file with " << head.num_notes << " notes " << std::endl;

	NoteRaw n;
	for (int i=0; i<head.num_notes; i++) {
	        f.read(reinterpret_cast<char*>(&n), NOTE_SIZE);

		if (!f) {
			std::cout << "Invalid save file " << filename << std::endl;
			exit(1);
			return;
		}
		if (n.time_due > max_tick) max_tick = n.time_due;
	        notes.push_back(CircleElement(i, {n.x, n.y}, {n.r / 255.0f, n.g / 255.0f, n.b / 255.0f}, n.time_due, '0' + (i % 10)));
		// printf("%f %f %ld\n", n.x, n.y, n.time_due);
	}
	Constraint c;
	for (int i=0; i<head.num_constraints; i++) {
	        f.read(reinterpret_cast<char*>(&c), CONSTRAINT_SIZE);

		if (!f) {
			std::cout << "Invalid save file " << filename << std::endl;
			exit(1);
			return;
		}
	        constraints.push_back(c);
		// printf("%f %f %ld\n", n.x, n.y, n.time_due);
	}


}
