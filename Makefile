IDIR =.
CC=gcc
CFLAGS=-I$(IDIR) -g -Wall -Wextra -pedantic -Wmissing-declarations

ODIR=.
LDIR =../lib

LIBS=-lX11 -lGL -lGLEW -lm

_DEPS = util.h config.h navigation.h
DEPS  = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o util.o config.o navigation.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

zooc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
