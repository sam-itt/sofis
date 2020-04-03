#TOP_SRCDIR=..
#SRCDIR=$(TOP_SRCDIR)/src
SRCDIR=.

CC=gcc
CFLAGS=-g3 -O0 `pkg-config glib-2.0 sdl2 SDL2_image --cflags` -I$(SRCDIR) 
LDFLAGS=-lz -lm `pkg-config glib-2.0 sdl2 SDL2_image --libs`
EXEC=test-sdl
SRC= $(wildcard $(SRCDIR)/*.c)
OBJ= $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper 

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC) 

