CXX = clang++

CXXFLAGS = -std=c++11 -g -Wall

CXXFLAGS += -I.
LDFLAGS += -lrtmidi liboscpack.a

SRC = envelope.cpp main.cpp
OBJ = ${SRC:.cpp=.o}

.cpp.o:
	${CXX} -c ${CXXFLAGS} -o $*.o $<

aarun: ${OBJ}
	${CXX} ${CXXFLAGS} -o aarun ${OBJ} ${LDFLAGS} 

clean:
	rm -rf aarun ${OBJ}
