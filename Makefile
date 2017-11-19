VERSION = 0.1
CC      = gcc

CFLAGS  = -Wall -O3
LDFLAGS = -lwiringPi

OBJ = main.o

prog: $(OBJ)
	$(CC) $(CFLAGS) -o lightc $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
