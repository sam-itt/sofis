include switches.defaults
sinclude switches.local

# Where to find resources/{fg-scenery,skybox} and shaders/
FGR_HOME=\"./fg-roam/src\"
SFS_HOME=\".\"

SRCDIR=.
FG_IO=$(SRCDIR)/fg-io
FGCONN=$(FG_IO)/flightgear-connector
FGTAPE=$(FG_IO)/fg-tape
FG_ROAM=$(SRCDIR)/fg-roam
CGLM=$(FG_ROAM)/lib/cglm/include
BNO080=$(SRCDIR)/sensors/bno080

ifeq ($(BUILD_MODE),debug)
	OPT_CFLAGS=-O0 -g3
else
	OPT_CFLAGS=-O2
endif

CC=gcc
CFLAGS=$(OPT_CFLAGS) `pkg-config glib-2.0 sdl2 SDL2_image libgps --cflags` \
	   -I$(SRCDIR) \
	   -I$(SRCDIR)/widgets \
	   -I$(SRCDIR)/dialogs \
	   -I$(SRCDIR)/sdl-pcf/src \
	   -I$(FGCONN) \
	   -I$(FGTAPE) \
	   -I$(BNO080) \
	   -I$(CGLM) \
	   -I$(FG_ROAM)/src \
	   -I/home/debian/git/sdl-gpu/include \
	   -DUSE_SGPU_TEXTURE=1 \
	   -DUSE_SDL_GPU=1 \
	   -DENABLE_DEBUG_TRIANGLE=0 \
	   -DENABLE_DEBUG_CUBE=0 \
	   -DFGR_HOME=$(FGR_HOME) \
	   -DSFS_HOME=$(SFS_HOME) \
	   -DBNO080_DEV=$(BNO080_DEV) \
	   -DENABLE_MOCK_GPS=$(ENABLE_MOCK_GPS) \
	   -DENABLE_PERF_COUNTERS=1 \
	   -DUSE_GLES=$(USE_GLES) \
	   -DENABLE_3D=$(ENABLE_3D) \
	   -DNO_PRELOAD=$(NO_PRELOAD) \
	   -DUSE_TINY_TEXTURES=$(TINY_TEXTURES) \
	   -DHAVE_MKDIR_P \
	   -DHAVE_CREATE_PATH \
	   -DHAVE_HTTP_DOWNLOAD_FILE \
	   -DHAVE_IGN_OACI_MAP=$(HAVE_IGN_OACI_MAP)
LDFLAGS=-lz -lm `pkg-config glib-2.0 sdl2 SDL2_image libgps --libs` -Wl,--as-needed -lSDL2_gpu -l$(GL_LIB) -lpthread -lcurl
EXEC=sofis
SRC= $(filter-out $(SRCDIR)/main.c $(SRCDIR)/testbench.c, $(wildcard $(SRCDIR)/*.c))
SRC+= $(wildcard $(SRCDIR)/widgets/*.c)
SRC+= $(wildcard $(SRCDIR)/dialogs/*.c)
SRC+= $(wildcard $(SRCDIR)/sdl-pcf/src/*.c)
SRC+= $(filter-out $(FGCONN)/fg-connector-test.c, $(wildcard $(FGCONN)/*.c))
SRC+= $(filter-out $(FGTAPE)/fg-tape-reader.c, $(wildcard $(FGTAPE)/*.c))
SRC+= $(wildcard $(SRCDIR)/sensors/*.c)
SRC+= $(filter-out $(BNO080)/test.c, $(wildcard $(BNO080)/*.c))
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

.PHONY: clean mrproper configure

configure:
	# Install dependencies
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev libsdl2-image-dev libglib2.0-dev libcurl4-openssl-dev libgps-dev
	# Install SDL_gpu
	git clone https://github.com/grimfang4/sdl-gpu.git sdl-gpu || (cd sdl-gpu && git pull)
	cd sdl-gpu && cmake -G "Unix Makefiles" && make && sudo make install
	# Initialize and update git submodules
	git submodule update --init --recursive
	# Configure sdl-pcf
	cd sdl-pcf && autoreconf -i && ./configure --with-texture=sdl_gpu && cd ..
	# Download and extract media
	wget -nc https://github.com/sam-itt/fg-roam/archive/media.tar.gz -O media.tar.gz
	tar -xf media.tar.gz --strip-components=1 -C fg-roam/

clean:
	rm -rf *.o sdl-pcf/src/*.o fg-roam/src/*.o fg-io/fg-tape/*.o sensors/*.o widgets/*.o dialogs/*.o

mrproper: clean
	rm -rf $(EXEC) media.tar.gz

