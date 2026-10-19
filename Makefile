CXX=clang++
CXXFLAGS=-Wall -Wextra -Werror -Wno-unused-private-field -g -std=c++20

SOURCES=main.cpp file.cpp exceptions.cpp
OBJECTS=$(patsubst %.cpp,%.o,${SOURCES})
TARGET=logconverter

all: ${TARGET}

${TARGET}: ${OBJECTS}
	${CXX} -o $@ $+

clean:
	rm -f ${OBJECTS}
	rm -f ${TARGET}
	rm -f .depends

-include .depends

.depends: Makefile ${SOURCES} ${UNITSOURCES}
	${CXX} -MM ${CXXFLAGS} ${SOURCES} > $@
