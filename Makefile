CC=g++ -std=c++14
OPT = -O3
LIBS = -lpthread

all: sheet

run: sheet
	./sheet

sheet: main.o sheet.o
	$(CC) $(OPT) -o sheet main.o sheet.o $(LIBS)

main.o: main.cpp parser.hpp 
	$(CC) -c $(OPT) main.cpp

sheet.o: sheet.cpp parser.hpp 
	$(CC) -c $(OPT) sheet.cpp
	
clean:
	rm -rf *.o sheet