# Grab the targets and sources as two batches
OBJECTS = $(patsubst src/%.cxx, build/%.o, $(wildcard src/*.cxx))
OBJECTS += build/json11.o
OBJ_VME = $(patsubst include/vme/%.c, build/%.o, $(wildcard include/vme/*.c))
#OBJ_DRS = $(patsubst src/drs/%.cpp, build/%.o, $(wildcard src/drs/*.cpp))
#OBJ_DRS += $(patsubst src/drs/%.c, build/%.o, $(wildcard src/drs/*.c))
DATADEF = include/common.hh include/common_extdef.hh
# LOGFILE = /var/log/lab-daq/fast-daq.log
# CONFDIR = /usr/local/opt/lab-daq/config

# Figure out the architecture
UNAME_S = $(shell uname -s)

# Clang compiler
ifeq ($(UNAME_S), Darwin)
	CXX = clang++
	CC  = clang
	CPPFLAGS = -DOS_DARWIN
	CXXFLAGS = -std=c++11
endif

# Gnu compiler
ifeq ($(UNAME_S), Linux)
	CXX = g++
	CC  = gcc
	CPPFLAGS = -DOS_LINUX
	CXXFLAGS = -std=c++0x
endif

ifdef DEBUG
	CXXFLAGS += -g -pg -fPIC -O3 -pthread
else
	CXXFLAGS += -fPIC -O3 -pthread
endif

# DRS flags
# CPPFLAGS += -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX

# ROOT libs and flags
CPPFLAGS += $(shell root-config --cflags)
LIBS = $(shell root-config --libs) -lCAENDigitizer -lzmq

CPPFLAGS += -Iinclude -Ijson11
LIBS += -lm -lzmq -lCAENDigitizer -lutil -lpthread

all: $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(TARGETS)

$(LOGFILE):
	@mkdir -p $(@D)
	@touch $@

$(CONFDIR):
	@mkdir -p $(@D)
	@cp -r config $(@D)/

include/%.hh: include/.default_%.hh
	cp $+ $@

build/json11.o: json11/json11.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: src/%.cxx $(DATADEF)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build/%.o: include/vme/%.c $(DATADEF)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# build/%.o: src/drs/%.cpp $(DATADEF)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# build/%.o: src/drs/%.c $(DATADEF)
# 	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

fe_%: modules/fe_%.cxx $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(DATADEF)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ \
	$(OBJECTS) $(OBJ_VME) $(OBJ_DRS) $(LIBS)

%_daq: modules/%_daq.cxx $(DATADEF)
	$(CXX) $< -o $@  $(CXXFLAGS) $(CPPFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS) $(OBJECTS) $(OBJ_VME) $(OBJ_DRS) build/json11.o
