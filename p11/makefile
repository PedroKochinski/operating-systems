CC = gcc
CFLAGS = -Wall -g

objs = ppos_core.o queue.o
 
all: tasks pingpong-prodcons

tasks: $(objs)

pingpong-prodcons: $(objs)

debug: CFLAGS += -DDEBUG
debug: clean tasks pingpong-prodcons

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks pingpong-prodcons