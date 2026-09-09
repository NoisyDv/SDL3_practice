CC=gcc
CFLAGS=-Iheader -Wall -Wextra -g
TARGET=out/game
SRC=$(wildcard src/*.c)

# ---- Static SDL3 + SDL3_image (baked into the binary) ----
# Built from source into third_party/sdl-static (SDL 3.4.16, image 3.4.6).
# System libs (X11/Wayland/audio) stay dynamic: SDL dlopens those backends
# at runtime, and no static archives for them exist on this machine.
SDL_VER=3.4.16
SDLIMAGE_VER=3.4.6
SDL_PREFIX=third_party/sdl-static
SDL_LIB=$(SDL_PREFIX)/lib/libSDL3.a
SDLIMAGE_LIB=$(SDL_PREFIX)/lib/libSDL3_image.a
SDL_PKGCONFIG=$(SDL_PREFIX)/lib/pkgconfig

CFLAGS += -I$(SDL_PREFIX)/include
# Explicit .a paths (prefix ships no .so, so these can only link static).
# Transitive deps come from the prefix .pc files, minus SDL itself.
SDL_DEPS := $(shell PKG_CONFIG_PATH=$(SDL_PKGCONFIG) pkg-config --static --libs sdl3 sdl3-image 2>/dev/null | sed -e 's/-lSDL3_image//g' -e 's/-lSDL3//g')
LDFLAGS=$(SDLIMAGE_LIB) $(SDL_LIB) $(SDL_DEPS) -lm

$(TARGET):$(SRC) $(SDL_LIB) $(SDLIMAGE_LIB)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

# Bootstrap: clone + build static SDL3/SDL3_image (needs network + cmake).
setup-static: $(SDL_LIB) $(SDLIMAGE_LIB)

$(SDL_LIB):
	set -e; \
	rm -rf third_party/src/SDL third_party/build-sdl; \
	git clone --depth 1 --branch release-$(SDL_VER) https://github.com/libsdl-org/SDL.git third_party/src/SDL; \
	cmake -S third_party/src/SDL -B third_party/build-sdl -G Ninja \
	  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(SDL_PREFIX) \
	  -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF; \
	cmake --build third_party/build-sdl -j$$(nproc); \
	cmake --install third_party/build-sdl; \
	rm -rf third_party/build-sdl

$(SDLIMAGE_LIB): $(SDL_LIB)
	set -e; \
	rm -rf third_party/src/SDL_image third_party/build-image; \
	git clone --depth 1 --branch release-$(SDLIMAGE_VER) https://github.com/libsdl-org/SDL_image.git third_party/src/SDL_image; \
	cmake -S third_party/src/SDL_image -B third_party/build-image -G Ninja \
	  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(SDL_PREFIX) \
	  -DCMAKE_INSTALL_PREFIX=$(SDL_PREFIX) \
	  -DBUILD_SHARED_LIBS=OFF -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF \
	  -DSDLIMAGE_VENDORED=OFF; \
	cmake --build third_party/build-image -j$$(nproc); \
	cmake --install third_party/build-image; \
	rm -rf third_party/build-image

clean:
	rm -f $(TARGET)

.PHONY: run setup-static clean
