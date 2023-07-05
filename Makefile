CC = g++
CFLAGS = `wx-config --cxxflags` -Wno-c++11-extensions -std=c++11
CLIBS = `wx-config --libs` -Wno-c++11-extensions -std=c++11
OBJ = rando.o logic.o patches.o rom.o

rando: $(OBJ)
	$(CC) -o rando $(OBJ) $(CLIBS)

rando.o: rando.cpp rando.h
	$(CC) -c rando.cpp $(CFLAGS)

logic.o: logic.cpp rando.h
	$(CC) -c logic.cpp $(CFLAGS)

patches.o: patches.cpp rando.h
	$(CC) -c patches.cpp $(CFLAGS)

rom.o: rom.cpp rom.h
	$(CC) -c rom.cpp $(CFLAGS)

.PHONY: clean
clean:
	-rm rando $(OBJ)