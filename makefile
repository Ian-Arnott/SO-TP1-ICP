# CC = gcc
# CFLAGS = -Wall -Wextra -Werror -g -std=c99

# all: aplication slave

# aplication: aplication.c
# 	$(CC) $(CFLAGS) aplication.c -o aplication

# slave: slave.c
# 	$(CC) $(CFLAGS) slave.c -o slave

# clean:
# 	rm -f aplication slave

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
LIBS = -lrt -lpthread

all: aplication view slave

aplication: aplication.o shm_lib.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

view: view.o shm_lib.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

slave: slave.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o aplication view slave
