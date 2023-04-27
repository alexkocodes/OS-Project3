CC = gcc
CFLAGS = -Wall -g -pthread

# list of source files
SRCS = reader.c writer.c coordinator.c

# list of object files
OBJS = $(SRCS:.c=.o)

# dependencies
# DEPS = 

# default target
all: reader writer coordinator

# compile the reader program
reader: reader.o 
	$(CC) $(CFLAGS) -o $@ $^

# compile the writer program
writer: writer.o 
	$(CC) $(CFLAGS) -o $@ $^

# compile the coordinator program
coordinator: coordinator.o 
	$(CC) $(CFLAGS) -o $@ $^


# clean up object files and executables
clean:
	rm -f $(OBJS) reader writer coordinator
