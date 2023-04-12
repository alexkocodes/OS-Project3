OBJS 	= reader.o writer.o
SOURCE	= reader.c writer.c
OUT  	= reader writer
CC	= gcc
FLAGS   = -g -c -pedantic -ansi  -Wall -std=c99
LIBS = -lm
# -g option enables debugging mode 
# -c flag generates object code for separate files

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $@ $(LIBS)

# create/compile the individual files >>separately<< 
reader.o: reader.c
	$(CC) $(FLAGS) reader.c $(LIBS)

writer.o: writer.c
	$(CC) $(FLAGS) writer.c $(LIBS)

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
