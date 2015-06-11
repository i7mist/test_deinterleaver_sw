CXX := clang++
CXXFLAGS := -g -Wall -stdlib=libc++
INC := libepee/app libepee/sPciDriver
INC_PARAMS := $(foreach d, $(INC), -I$d)
OBJPATH := libepee/app/
OBJ := sPcie.o sPcieZerocopy.o
OBJ_PARAMS := $(foreach d, $(OBJ), $(OBJPATH)$d)

all: main

main: sw.cpp $(OBJ_PARAMS)
	$(CXX) $(CXXFLAGS) $(INC_PARAMS) $^ -o $@

.PHONY: clean all

clean:
	rm -f *.o mainb