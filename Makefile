CXX = clang++

CXXFLAGS = -std=c++11 -g -Wall

CXXFLAGS += `pkg-config liblo --cflags`
LDFLAGS += `pkg-config liblo --libs`

SRC = main.cpp osc_server.cpp
OBJ = ${SRC:.cpp=.o}

.cpp.o:
	${CXX} -c ${CXXFLAGS} -o $*.o $<

aarun: ${OBJ}
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o aarun ${OBJ}

clean:
	rm -rf aarun ${OBJ}
