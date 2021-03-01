#TOP_SRCDIR=..
#SRCDIR=$(TOP_SRCDIR)/src
SRCDIR=.
FG_IO=$(SRCDIR)/fg-io
FGCONN=$(FG_IO)/flightgear-connector
FGTAPE=$(FG_IO)/fg-tape
FG_ROAM=$(SRCDIR)/fg-roam
CGLM=$(FG_ROAM)/lib/cglm/include
ENABLE_3D=1

CC=gcc
CFLAGS=-g3 -O0 `pkg-config glib-2.0 sdl2 SDL2_image libgps --cflags` \
	   -I$(SRCDIR) \
	   -I$(SRCDIR)/sdl-pcf/src \
	   -I$(FGCONN) \
	   -I$(FGTAPE) \
	   -I$(CGLM) \
	   -I$(FG_ROAM)/src \
	   -DUSE_SGPU_TEXTURE=1 \
	   -DUSE_SDL_GPU=1 \
	   -DENABLE_DEBUG_TRIANGLE=0 \
	   -DENABLE_DEBUG_CUBE=0 \
	   -DSHADER_ROOT=\"/home/samuel/dev/efis-hud/fg-roam/src/shaders/gl\" \
	   -DTERRAIN_ROOT=\"/home/samuel/dev/Terrain\" \
	   -DSKY_ROOT=\"/home/samuel/dev/efis-hud/fg-roam/src\" \
	   -DTEX_ROOT=\"/home/samuel/dev/textures\" \
	   -DTILES_ROOT=\"/home/samuel/ecw2/oaci-tiles\" \
	   -DENABLE_PERF_COUNTERS=1 \
	   -DUSE_GLES=0 \
	   -DENABLE_3D=$(ENABLE_3D)
LDFLAGS=-lz -lm `pkg-config glib-2.0 sdl2 SDL2_image libgps --libs` -Wl,--as-needed -lSDL2_gpu -lGL -lpthread
EXEC=test-sdl
SRC= $(filter-out $(SRCDIR)/main.c $(SRCDIR)/testbench.c, $(wildcard $(SRCDIR)/*.c))
SRC+= $(wildcard $(SRCDIR)/sdl-pcf/src/*.c)
SRC+= $(filter-out $(FGCONN)/fg-connector-test.c, $(wildcard $(FGCONN)/*.c))
SRC+= $(filter-out $(FGTAPE)/fg-tape-reader.c, $(wildcard $(FGTAPE)/*.c))
SRC+= $(wildcard $(SRCDIR)/sensors/*.c)
ifeq ($(ENABLE_3D), 1)
SRC+= $(filter-out $(FG_ROAM)/src/view-gl.c, $(wildcard $(FG_ROAM)/src/*.c))
endif
OBJ= $(SRC:.c=.o)
MAIN_OBJ=main.o
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
	rm -rf *.o sdl-pcf/src/*.o fg-roam/src/*.o fg-io/fg-tape/*.o sensors/*.o

mrproper: clean
	rm -rf $(EXEC)

