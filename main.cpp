#include <iostream>
#include <fstream>

#include "parser.hpp"

using std::ifstream;

int main()
{
	string fname = "input.txt";
	ifstream ifile;
	ifile.open(fname);
	std::cerr << "Reading file: "  << fname << " : " << strerror(errno) << endl;

	int numThreads = 8;
	int err = 0;
	Interpretator interpr(ifile, err, numThreads);
	if (err) {
		cout << "Error in input format\n";
	}
	return 0;
}
