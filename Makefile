VERSION = 0.2
CC      = gcc

CFLAGS  = -Wall -O3
LDFLAGS = -lpigpio -lpaho-mqtt3c

OBJ = main.o

prog: $(OBJ)
	$(CC) $(CFLAGS) -o piPinConnector $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
