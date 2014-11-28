CXX = clang++

CXXFLAGS = -std=c++11 -g -Wall

CXXFLAGS += -I. -Ioscpack/
LDFLAGS += -lrtmidi

SRC = envelope.cpp main.cpp \
			oscpack/ip/posix/NetworkingUtils.cpp \
			oscpack/ip/posix/UdpSocket.cpp \
			oscpack/ip/IpEndpointName.cpp \
			oscpack/osc/OscOutboundPacketStream.cpp \
			oscpack/osc/OscReceivedElements.cpp \
			oscpack/osc/OscTypes.cpp \
			oscpack/osc/OscPrintReceivedElements.cpp
OBJ = ${SRC:.cpp=.o}

.cpp.o:
	${CXX} -c ${CXXFLAGS} -o $*.o $<

aarun: ${OBJ}
	${CXX} ${CXXFLAGS} -o aarun ${OBJ} ${LDFLAGS} 

clean:
	rm -rf aarun ${OBJ}
