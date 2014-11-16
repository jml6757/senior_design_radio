#**********************************************************************
# File: Makefile
#
# Description: Generic C project Makefile for linux applications.
#              Assumes a flat directory structure and only one 
#              main function. Change BIN to desired executable name.
#
# Author: Jonathan Lunt (jml6757@rit.edu)
#**********************************************************************

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
BIN  = radio_tx

CFLAGS  = -g -O2 -Wall

all: $(BIN)

clean:
	rm -rf *.o $(BIN)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
