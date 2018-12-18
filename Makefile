.SUFFIXES : .c .o

CC = /usr/bin/gcc
CFLAGS = -g -c -Wall -D_GNU_SOURCE
LDFLAGS= -Wall

LIBFILE=

SRC= cachesim.c

OBJ= $(addsuffix .o, $(basename $(SRC)))

TARGET= cachesim

all: $(TARGET)

cachesim: cachesim.o
	$(CC) $(LDFLAGS) $(LIBDIR) $(LIBFILE) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) $(INCDIR) $<

clean:
	rm -rf $(OBJ) $(TARGET) core
