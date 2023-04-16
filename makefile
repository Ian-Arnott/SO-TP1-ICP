CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
LIBS = -lrt -lpthread

all: aplication slave view


aplication: aplication.o lib.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

slave: slave.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

view: view.o lib.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o aplication view slave
