CC = gcc
CFLAGS = -Wall -DDEBUG

objs = ppos_core.o queue.o
 
all: tasks teste

tasks: $(objs)

teste: $(objs)

clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f tasks teste