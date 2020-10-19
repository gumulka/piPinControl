VERSION = 0.2

CFLAGS  ?= -Wall -O3
LDFLAGS += -lpigpio -lpaho-mqtt3c

OBJ = main.o

BIN = piPinConnector

.PHONY: all

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(OBJ) $(BIN)
