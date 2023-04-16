OBJS 	= reader writer coordinator
SOURCE	= reader.c writer.c coordinator.c
CC	= gcc
FLAGS   = -g -c -pedantic -ansi  -Wall -std=c99
LIBS = -lm
# -g option enables debugging mode 
# -c flag generates object code for separate files

all: reader writer coordinator
# create/compile the individual files >>separately<< 
reader: reader.c
	gcc -o reader reader.c
	
writer: writer.c
	gcc -o writer writer.c

writer: coordinator.c
	gcc -o coordinator coordinator.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
