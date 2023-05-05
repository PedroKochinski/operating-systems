CC = gcc
CFLAGS = -Wall -g

objs = ppos_core.o queue.o
 
all: tasks teste-stress

tasks: $(objs)

teste-stress: $(objs)

debug: CFLAGS += -DDEBUG
debug: clean tasks teste-stress

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks teste-stress