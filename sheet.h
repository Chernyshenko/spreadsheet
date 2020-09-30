#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

using Value = double;

class SpreadSheet;
class Cell;
class Formula;


class Formula {
public:
	void addCell(SpreadSheet* sheet, std::string& s);
	void addAcc(double a);
	std::vector< std::reference_wrapper<Cell> >& getDependents();
	Value evaluate(SpreadSheet* sheet);
	bool Independent() const;
	Value getAcc() const;

private:
	std::vector< std::reference_wrapper<Cell> > cells;
	Value acc = 0;
	Value value = 0;
};
struct CellPointerHasher
{
	std::size_t operator()(const Cell* k) const;
};
class Cell {
private:

	Formula formula;
	SpreadSheet* sheet;
	Value value;
	bool isReady;
public:
	int x, y;
	std::unordered_set<Cell*, CellPointerHasher> observing;
	std::unordered_set<Cell*, CellPointerHasher> observers;
	Cell(SpreadSheet* sheet_, const std::string& cellLetter, const std::string& cellNumber);

	Value getValue() const;
	bool IsReady() const;
	bool observingReady();
	void putReady(bool t);
	void update();
	void notifyObservers();

	void PutFormula(Formula& e);
	Value evaluateFormula();

	bool operator ==(const Cell &b) const;
};

struct CellHasher
{
	std::size_t operator()(const Cell& k) const
	{
		return ((std::hash<Value>()(k.x) ^ (std::hash<Value>()(k.y) << 1)) >> 1);
	}
};



class SpreadSheet {
private:
	int evaluationCounter = 0;
	//std::vector<Cell> cells;
	std::unordered_set<Cell, CellHasher> all_cells;
	std::queue<Cell*> evaluationRequired;
	
	//std::map< int2, int2> observing;
	//ThreadSafeQueue<std::reference_wrapper<Cell> > evaluationRequired;
public:
	std::vector<std::thread> pool;
	std::mutex formula_mutex;
	std::mutex observing_mutex;
	std::condition_variable newFormula;
	bool stop_pool;
	size_t poolSize;

	SpreadSheet(size_t numThreads = 2);
	~SpreadSheet();
	Cell& GetCell(const std::string& cellLetter, const std::string& cellNumber);
	void cellRequiresEvaluation(Cell& cell);
	void printValues();
	void printToFile(std::string fname = "output.txt");
	bool checkValues();

	void deleteCellObserver(Cell& c, Cell& observer);
	void addCellObserver(Cell& c, Cell& observer);
};
