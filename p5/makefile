CC = gcc
CFLAGS = -Wall -g

objs = ppos_core.o queue.o
 
all: tasks teste

tasks: $(objs)

teste: $(objs)

debug: CFLAGS += -DDEBUG
debug: clean tasks teste

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks teste