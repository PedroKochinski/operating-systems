CC = gcc
CFLAGS = -Wall -g -DDEBUG 
 
objs = ppos_core.o
 
all: tasks teste1

tasks: $(objs)

teste1: $(objs)

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks