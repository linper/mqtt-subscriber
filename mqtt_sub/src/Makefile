DEPS = $(wildcard *.h)
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
TARGET = mqtt_sub
LDFLAGS+=-luci -lmosquitto -ljson-c -lblobmsg_json -lsqlite3
CFLAGS = -Wall -Wextra -g
RM = rm -f

all: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(OBJ) $(TARGET)