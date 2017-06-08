# Makefile for EEC277-Project
# for compiling outside of Visual Studio

CC = g++
CFLAGS = -g -Wall -Wno-unknown-pragmas -Werror -std=c++11

LIBS += -lstdc++

ALL = TestGenerator
#ALL = TriangleTraversal genTris GrabTriangles

all: $(ALL)

TestGenerator: TestGenerator.cpp CircuitTypes.cpp
	@echo "-------------------------------"
	@echo "*** Building $@ ***"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "*** Complete! ***"
	@echo "-------------------------------"

# clean targets
clean:
	@echo "-------------------------------"
	@echo "*** Cleaning Files..."
	rm -rf *.o *.dSYM *.DS_STORE $(ALL)
	@echo "-------------------------------"
