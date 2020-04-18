#TOP_SRCDIR=..
#SRCDIR=$(TOP_SRCDIR)/src
SRCDIR=.

CC=gcc
CFLAGS=-g3 -O0 `pkg-config sdl2 SDL2_image SDL2_ttf --cflags` -I$(SRCDIR)
LDFLAGS=-lz -lm `pkg-config sdl2 SDL2_image SDL2_ttf --libs` -Wl,--as-needed
EXEC=test-sdl
#SRC= $(wildcard $(SRCDIR)/*.c)
SRC= $(filter-out $(SRCDIR)/main.c $(SRCDIR)/testbench.c, $(wildcard $(SRCDIR)/*.c))
OBJ= $(SRC:.c=.o)
MAIN_OBJ= main.o
TEST_OBJ=testbench.o

all: $(EXEC)

$(EXEC): $(OBJ) $(MAIN_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

testbench: $(OBJ) $(TEST_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

