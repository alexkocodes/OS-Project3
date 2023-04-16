CC = gcc
CFLAGS = -Wall -g

# list of source files
SRCS = reader.c writer.c coordinator.c Hashmap.c

# list of object files
OBJS = $(SRCS:.c=.o)

# dependencies
DEPS = Hashmap.h

# default target
all: reader writer coordinator

# compile the reader program
reader: reader.o Hashmap.o
	$(CC) $(CFLAGS) -o $@ $^

# compile the writer program
writer: writer.o Hashmap.o
	$(CC) $(CFLAGS) -o $@ $^

# compile the coordinator program
coordinator: coordinator.o Hashmap.o
	$(CC) $(CFLAGS) -o $@ $^

# compile the Hashmap module
Hashmap.o: Hashmap.c Hashmap.h
	$(CC) $(CFLAGS) -c $<

# clean up object files and executables
clean:
	rm -f $(OBJS) reader writer coordinator
