#TOP_SRCDIR=..
#SRCDIR=$(TOP_SRCDIR)/src
SRCDIR=.
FG_IO=$(SRCDIR)/fg-io
FGCONN=$(FG_IO)/flightgear-connector
FGTAPE=$(FG_IO)/fg-tape

CC=gcc
CFLAGS=-g3 -O0 `pkg-config sdl2 SDL2_image --cflags` \
	   -I$(SRCDIR) \
	   -I$(SRCDIR)/sdl-pcf/src \
	   -I$(FGCONN) \
	   -I$(FGTAPE) \
	   -DUSE_SGPU_TEXTURE=1 \
	   -DUSE_SDL_GPU=1
LDFLAGS=-lz -lm `pkg-config sdl2 SDL2_image --libs` -Wl,--as-needed -lSDL2_gpu
EXEC=test-sdl
#SRC= $(wildcard $(SRCDIR)/*.c)
SRC= $(filter-out $(SRCDIR)/main.c $(SRCDIR)/testbench.c, $(wildcard $(SRCDIR)/*.c))
SRC+= $(wildcard $(SRCDIR)/sdl-pcf/src/*.c)
SRC+= $(filter-out $(FGCONN)/fg-connector-test.c, $(wildcard $(FGCONN)/*.c))
SRC+= $(filter-out $(FGTAPE)/fg-tape-reader.c, $(wildcard $(FGTAPE)/*.c))
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
	rm -rf *.o sdl-pcf/src/*.o

mrproper: clean
	rm -rf $(EXEC)

