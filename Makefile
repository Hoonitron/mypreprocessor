CC=gcc
CFLAGS=-Wall -Wextra
TARGET=mypreprocessor
OBJS=main.o preprocessor.o comment_removal.o file_inclusion.o macro_handler.o buffer.o
HEADERS=$(wildcard *.h)

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o:%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY:all clean
