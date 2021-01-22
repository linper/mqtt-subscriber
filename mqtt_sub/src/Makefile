DEPS = $(wildcard *.h)
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
TARGET = mqtt_sub
LDFLAGS+=-luci -lmosquitto -lsqlite3 -ljson-c
CFLAGS = -Wall -Wextra -g
# DEBUG = -DDEBUG
DEBUG =
RM = rm -f

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEBUG)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(OBJ) $(TARGET)