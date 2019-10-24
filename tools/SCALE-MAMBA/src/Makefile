
include ../CONFIG.mine

CC = g++

CFLAGS = -Wall -std=c++11 -pedantic -Wextra -g -pthread -I$(ROOT)/src -maes -mpclmul -msse4.1 -mavx -march=core2 $(FLAGS) $(OPT) -I$(OSSL)/include 
CPPFLAGS = $(CFLAGS) 
LDLIBS = -L/$(OSSL)/lib -lm -lssl -lcrypto -lmpirxx -lmpir -lcryptopp $(LDFLAGS)

all: libmpc.a

OFFLINE = $(patsubst %.cpp,%.o,$(wildcard Offline/*.cpp))
ONLINE = $(patsubst %.cpp,%.o,$(wildcard Online/*.cpp))
SYSTEM = $(patsubst %.cpp,%.o,$(wildcard System/*.cpp))
TOOLS = $(patsubst %.cpp,%.o,$(wildcard Tools/*.cpp))
MATH = $(patsubst %.cpp,%.o,$(wildcard Math/*.cpp))
LSSS = $(patsubst %.cpp,%.o,$(wildcard LSSS/*.cpp))
OT = $(patsubst %.cpp,%.o,$(wildcard OT/*.cpp))
PROCESSOR = $(patsubst %.cpp,%.o,$(wildcard Processor/*.cpp))
IO = $(patsubst %.cpp,%.o,$(wildcard Input_Output/*.cpp))
FHE = $(patsubst %.cpp,%.o,$(wildcard FHE/*.cpp))
GC = $(patsubst %.cpp,%.o,$(wildcard GC/*.cpp))
LOCAL_FUNC = $(patsubst %.cpp,%.o,$(wildcard Local/*.cpp))
EXCEPTIONS = $(patsubst %.cpp,%.o,$(wildcard Exceptions/*.cpp))
PLAYER = $(patsubst %.cpp,%.o,$(wildcard Player/*.cpp))

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $< 

libmpc.a: $(SYSTEM) $(MATH) $(TOOLS) $(LSSS) $(OFFLINE) $(ONLINE) $(OT) $(PROCESSOR) $(IO) $(FHE) $(GC) $(LOCAL_FUNC) $(EXCEPTIONS) $(PLAYER)
	$(AR) -crs libmpc.a $^
	mv libmpc.a ../

clean:
	- rm *.o libmpc.a
	- cd System ; rm *.o
	- cd Tools ; rm *.o
	- cd Math ; rm *.o
	- cd LSSS ; rm *.o
	- cd Processor ; rm *.o
	- cd Offline ; rm *.o
	- cd Online ; rm *.o
	- cd Input_Output ; rm *.o
	- cd FHE ; rm *.o
	- cd OT ; rm *.o
	- cd GC ; rm *.o
	- cd Local ; rm *.o
	- cd Exceptions ; rm *.o
	- cd Player ; rm *.o
