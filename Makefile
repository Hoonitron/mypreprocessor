CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = mypreprocessor
SRCS = main.c preprocessor.c comment_removal.c file_inclusion.c macro_handler.c buffer.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
