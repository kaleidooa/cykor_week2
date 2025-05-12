TARGET = shell

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

shell: main.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

.PHONY: clean
clean:
	rm -f $(TARGET) main.o
