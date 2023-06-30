CC = gcc
CFLAGS = -Wall -g -lrt

objs = ppos_core.o queue.o ppos_disk.o disk.o
 
all: tasks teste

tasks: $(objs) 

teste: $(objs) -lm

debug: CFLAGS += -DDEBUG
debug: clean tasks teste

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks teste