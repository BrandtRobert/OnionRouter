CXX=c++
LDFLAGS=
CFLAGS=-Wall -g -c -std=c++11 -stdlib=libc++
SOURCES=main.cpp steppingstone.cpp
OBJECTS=main.o steppingstone.o
HEADERS=steppingstone.h
EXECUTABLE=test
TARFILE=onion-reutimann.tar

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm *.o chat $(TARFILE)

tar:
	tar -cvf $(TARFILE) *.cpp *.h Makefile
