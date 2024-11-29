CC=gcc
CFLAGS=-Wall -Wextra -pedantic -O3
LDFLAGS=-lX11 -lGL -lGLEW -lm -lXrandr

SRC=$(wildcard src/*.c)
INCLUDES=-I.
OBJ=$(SRC:.c=.o)

CONFIG_FILES=$(wildcard *.glsl) $(wildcard *.conf)

EXEC=zooc

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install:
	install -Dm755 $(EXEC) $(DESTDIR)/usr/bin/$(EXEC)
	install -Dm644 ./LICENSE "$(DESTDIR)/usr/share/licenses/zooc/LICENSE"
	install -d $(DESTDIR)/etc/$(EXEC)

	for file in $(CONFIG_FILES); do \
		install -Dm644 $$file $(DESTDIR)/etc/$(EXEC)/$$(echo $$file | sed 's/\.example//'); \
	done

clean:
	rm -f $(OBJ) $(EXEC)
