INCLUDE = queue.c queue.h testafila.c
CFLAG = -Wall -g -DDEBUG

all: queue

queue: $(INCLUDE)
	gcc $(CFLAG) -o queue $(INCLUDE) $(LDLIBS)

clean:
	-rm -f *.o

purge: clean
	-rm -f boulderdash *.o
