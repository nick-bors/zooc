IDIR =.
CC=gcc
CFLAGS=-I$(IDIR) -g

ODIR=.
LDIR =../lib

LIBS=-lX11 -lGL -lglut

_DEPS = util.h config.h
DEPS  = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o util.o config.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

zooc: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
