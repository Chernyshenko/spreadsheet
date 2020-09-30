#include <vector>
#include <iostream>
#include <chrono>
#include <limits>
#include <cmath>
#include "sheet.h"
using namespace std;
Value VALUE_INITIAL = NAN; // std::numeric_limits<Value>::min();

Value Sum(Value a, Value b) {
	//this_thread::sleep_for(std::chrono::milliseconds(1));
	return a + b;
}

std::size_t CellPointerHasher::operator()(const Cell* k) const
{
	return ((std::hash<Value>()(k->x) ^ (std::hash<Value>()(k->y) << 1)) >> 1);
}
bool Cell::operator ==(const Cell &b) const {
	return (x == b.x && y == b.y);
}
Cell::Cell(SpreadSheet* sheet_, const std::string& cellLetter, const std::string& cellNumber) 
	: sheet(sheet_)
	, value(VALUE_INITIAL)
	, isReady(false)
{
	x = cellLetter[0] - 'A'; //assume we have 1-letter only, for simplicity
	y = std::stoi(cellNumber);
}

bool Cell::IsReady() const {
	return isReady;
}
void Cell::putReady(bool t) {
	isReady = t;
}
bool Cell::observingReady() {
	for (auto cell : observing) {
		if (!cell->IsReady()) {
			putReady(false);
			return false;
		}
	}
	return true;
}

void Cell::PutFormula(Formula& e) {
	//check previous
	if (!formula.Independent()) {
		auto dependents = formula.getDependents();
		for (auto& c : dependents) {
			sheet->deleteCellObserver(c, *this);
		}
	}
	formula = e;
	auto dependents = e.getDependents();
	for (auto& c : dependents) {
		sheet->addCellObserver(c, *this);
	}
	sheet->cellRequiresEvaluation(*this);
}
void Cell::update() {
	sheet->cellRequiresEvaluation(*this);
}
Value Cell::evaluateFormula()
{
	Value newValue = formula.evaluate(sheet);
	if (newValue != value) {
		value = newValue;
		putReady(true);
		notifyObservers();
	}
	return value;
}
void Cell::notifyObservers() {
	for (auto c : observers) {
		c->update();
	}
}
Value Cell::getValue() const {
	return value;
}


void Formula::addCell(SpreadSheet* sheet, std::string& s) {
	cells.emplace_back(sheet->GetCell(s, std::string(s.begin() + 1, s.end())));
}
void Formula::addAcc(double a) {
	acc = Sum(acc, a);
}
Value Formula::getAcc() const {
	return acc;
}
std::vector< std::reference_wrapper<Cell> >& Formula::getDependents() {
	return cells;
}
bool Formula::Independent() const {
	return cells.empty();
}
Value Formula::evaluate(SpreadSheet* sheet) {
	Value res = acc;
	for (Cell& c : cells) {
		res = Sum(res, c.getValue());
	}
	if (res != value) {
		value = res;
	}
	return value;
}

SpreadSheet::SpreadSheet(size_t numThreads) 
	: evaluationCounter(0)
	, poolSize(numThreads)
	, stop_pool(false)
{
	
	for (size_t i = 0; i < poolSize; i++) {
		pool.emplace_back([this]() {
			while (true) {
				Cell* c;
				{
					std::unique_lock<std::mutex> lock(formula_mutex);
					newFormula.wait(lock,
						[this]()
					{
						return !evaluationRequired.empty() || stop_pool;
					});
					if (evaluationRequired.empty() || stop_pool) {
						return;
					}

					c = evaluationRequired.front();
					evaluationRequired.pop();
				}
				c->evaluateFormula();
			}
		});
	}

}

SpreadSheet::~SpreadSheet() {
	{
		std::unique_lock<std::mutex> lock(formula_mutex);
		stop_pool = true;
	}
	newFormula.notify_all();
	for (auto& t : pool)
	{
		t.join();
	}
	//printValues();
	printToFile();
}

Cell& SpreadSheet::GetCell(const std::string& cellLetter, const std::string& cellNumber) {
	Cell cell(this, cellLetter, cellNumber);
	//auto it = std::find(cells.begin(), cells.end(), cell);
	auto it = all_cells.find(cell);
	if (it == all_cells.end()) {
		it = all_cells.insert(cell).first;
	}
	return (Cell&)(*it);
}
void SpreadSheet::cellRequiresEvaluation(Cell& c) {
	std::unique_lock<std::mutex> lock(formula_mutex);
	bool cobservingReady = c.observingReady();
	
	if (cobservingReady) { 
		evaluationRequired.push(&c);
		newFormula.notify_one(); //all?
	}
}
char toLetter(int i) {
	return 'A' + i;
}
void SpreadSheet::printValues() {
	for (auto& c : all_cells) {
		std::cout <<  toLetter(c.x) << c.y << " = " << c.getValue() << std::endl;
	}
}

void SpreadSheet::printToFile(string fname) {
	std::ofstream out;
	out.open(fname);
	for (auto& c : all_cells) {
		out << toLetter(c.x) << c.y << " = " << c.getValue() << std::endl;
	}
	std::cout << "Saved into " << fname << endl;
}
bool SpreadSheet::checkValues() {
	for (auto& c : all_cells) {
		if (!c.IsReady()) return false;
	}
	return true;
}

void SpreadSheet::deleteCellObserver(Cell& c, Cell& observer) {
	std::unique_lock<std::mutex> lock(observing_mutex);
	observer.observing.erase(&c);
	c.observers.erase(&observer);
}

void SpreadSheet::addCellObserver(Cell& c, Cell& observer) {
	std::unique_lock<std::mutex> lock(observing_mutex);
	observer.observing.insert(&c);
	c.observers.insert(&observer);
}