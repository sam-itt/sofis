RES_HOME=/home/samuel/dev
ENABLE_3D=1
USE_GLES=0
USE_TINY_TEX=0
NO_PRELOAD=0

SRCDIR=.
FG_IO=$(SRCDIR)/fg-io
FGCONN=$(FG_IO)/flightgear-connector
FGTAPE=$(FG_IO)/fg-tape
FG_ROAM=$(SRCDIR)/fg-roam
CGLM=$(FG_ROAM)/lib/cglm/include

ifeq ($(USE_GLES),1)
	SHADER_DIR=gles
else
	SHADER_DIR=gl
endif

ifeq ($(USE_TINY_TEX),1)
	TEX_DIR=small
else
	TEX_DIR=full
endif

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
	   -DSHADER_ROOT=\"fg-roam/src/shaders/$(SHADER_DIR)\" \
	   -DSKY_ROOT=\"fg-roam/src\" \
	   -DTERRAIN_ROOT=\"$(RES_HOME)/Terrain\" \
	   -DTEX_ROOT=\"$(RES_HOME)/textures/$(TEX_DIR)\" \
	   -DMAPS_HOME=\"$(RES_HOME)/maps\" \
	   -DENABLE_PERF_COUNTERS=1 \
	   -DUSE_GLES=$(USE_GLES) \
	   -DENABLE_3D=$(ENABLE_3D) \
	   -DNO_PRELOAD=$(NO_PRELOAD) \
	   -DHAVE_MKDIR_P \
	   -DHAVE_CREATE_PATH \
	   -DHAVE_HTTP_DOWNLOAD_FILE
LDFLAGS=-lz -lm `pkg-config glib-2.0 sdl2 SDL2_image libgps --libs` -Wl,--as-needed -lSDL2_gpu -lGL -lpthread -lcurl
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

