CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -std=c99

all: aplication slave

aplication: aplication.c
	$(CC) $(CFLAGS) aplication.c -o aplication

slave: slave.c
	$(CC) $(CFLAGS) slave.c -o slave

clean:
	rm -f aplication slave

