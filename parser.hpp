#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <string.h>
#include <unordered_map>

#include "sheet.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using Value = double;

const char space = ' ';
const char mminus = '-';
const char sep = '.';


class LineParser {
public:
	LineParser(SpreadSheet* sheet_) :
	sheet(sheet_) {

	}
	~LineParser() {
		cur = nullptr;
	}
	int ParseLine(const string& cur_, string& rv);
private:
	const char* cur;
	SpreadSheet* sheet;
	std::string ParseV();

	std::string ParseOperation(std::vector<char>& operations);
	std::string ParseSumOperation();
	std::string ParseEqOperation();
	Cell& ParseCell();
	void ParseFormula(Formula& f);
	void ParseFormula1(Formula& f);
	bool sIsNum(const string& s);
	bool sIsAlpha(const string& s);
};

bool LineParser::sIsNum(const string& s) {
	if (s.empty()) return false;
	if (isdigit(s[0])) return true;
	return false;
}
bool LineParser::sIsAlpha(const string& s) {
	if (s.empty()) return false;
	if (isalpha(s[0])) return true;
	return false;
}


std::string LineParser::ParseOperation(std::vector<char>& operations) {

	while (*cur == space) cur++;
	for (auto& op : operations) {
		if ((*cur) == op) {
			cur++;
			return std::string(1, op);
		}
	}
	return "";
}
std::string LineParser::ParseSumOperation() {
	std::vector<char> operations = { '+', '-' };
	return ParseOperation(operations);
}
std::string LineParser::ParseEqOperation() {
	std::vector<char> operations = { '=' };
	return ParseOperation(operations);
}

Cell& LineParser::ParseCell() {
	while (*cur == space) cur++;
	std::ostringstream cellLetter;
	std::ostringstream cellNum;
	while (isalpha(*cur)) {
		cellLetter << *cur;
		cur++;
	}
	while (isdigit(*(cur))) {
		cellNum << *(cur);
		cur++;
	}
	return sheet->GetCell(cellLetter.str(), cellNum.str());
}

std::string LineParser::ParseV() {

	while (*cur == space) cur++;

	std::ostringstream snum;
	if (*cur == mminus) {
		snum << *(cur);
		cur++;
		while (*cur == space) cur++;
	}

	while (isdigit(*(cur))) {
		snum << *(cur);
		cur++;
	}

	if (!snum.str().empty()) {
		return snum.str();
	}

	while (isalpha(*cur) || isdigit(*cur)) {
		snum << *(cur);
		cur++;
	}
	if (!snum.str().empty()) {
		return snum.str();
	}

	return "";
}

void LineParser::ParseFormula(Formula& f) {

	string v = ParseV(); //Cell or Number
	if (v.empty()) return;
	if (sIsNum(v)) {
		double acc = std::stod(v);
		f.addAcc(acc);
	}
	else if (sIsAlpha(v)) {
		f.addCell(sheet, v);
	}
	ParseFormula1(f);
	return;
}


void LineParser::ParseFormula1(Formula& f) {
	auto operation = ParseSumOperation();
	if (operation.empty()) return;
	ParseFormula(f);
}
int lineId = 0;
int LineParser::ParseLine(const string& cur_, string& rv) {
	if (lineId % 1000 == 0) std::cout << lineId << " line parsing\r";
	lineId++;
	cur = cur_.c_str();
	Cell& cellInput = ParseCell();

	auto eq = ParseEqOperation();
	if (eq != "=") throw std::runtime_error("Error in input");
	
	Formula f;
	ParseFormula(f);
	cellInput.PutFormula(f);

	return 0;
}


class Interpretator
{
public:
	Interpretator(std::ifstream& f, int& err, int numThreads);
	~Interpretator() {};
};

Interpretator::Interpretator(std::ifstream& f, int& err, int numThreads) {

	clock_t begin_time = clock();
	SpreadSheet sheet(numThreads);
	LineParser parser(&sheet);
	
	string line;
	try {
		while (std::getline(f, line)) {
			string v;
			auto res = parser.ParseLine(line, v);
		}
	}
	catch (std::exception& e) {
		std::cout << e.what() << '\n';
		err = -1;
	}
	clock_t parsing_time = clock();
	cout << "Parsing done in " << float(parsing_time - begin_time) / CLOCKS_PER_SEC << " sec\n";
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	while (!sheet.checkValues()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	clock_t total_time = clock();
	cout << "Values are ready, saving into the file. Total time is " << float(total_time - begin_time) / CLOCKS_PER_SEC << " sec\n";

	//sheet.printValues();
}
