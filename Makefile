CXX=c++

EXE = test

SRC_DIR = src
OBJ_DIR = bin

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CFLAGS=-Wall -g -c -std=c++11 -stdlib=libc++
LDFLAGS=

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	@echo "Object files " $^
	$(CXX) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Building file " $<
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o