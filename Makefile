CC = gcc
CFLAGS = -Wall -Wextra -ggdb -I./include/ -MMD -MP
LDFLAGS =
TARGET = build/main.out

SRC = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, build/%.o, $(SRC))
DEPS     = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)
