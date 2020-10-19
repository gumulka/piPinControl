VERSION = 0.2

CFLAGS  ?= -Wall -O3
LDFLAGS += -lpigpio -lpaho-mqtt3c

OBJ = main.o

piPinConnector: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
