CXX ?= g++
PKG_CONFIG ?= pkg-config
ifeq ($(strip $(PKG_CONFIG)),)
PKG_CONFIG := pkg-config
endif
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_ttf)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 SDL2_ttf)
ZIP_CFLAGS := $(shell $(PKG_CONFIG) --cflags libzip)
ZIP_LIBS := $(shell $(PKG_CONFIG) --libs libzip)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -I./src $(SDL_CFLAGS) $(ZIP_CFLAGS)
PAK_DIR := dist/RetroRead
PAK_ASSETS_DIR := $(PAK_DIR)/assets

SOURCES := \
	src/app/main.cpp \
	src/app/Application.cpp \
	src/audio/TextBlipPlayer.cpp \
	src/core/BookLibrary.cpp \
	src/core/BookCache.cpp \
	src/core/ProgressStore.cpp \
	src/core/SettingsStore.cpp \
	src/epub/EpubCompiler.cpp \
	src/epub/EpubArchive.cpp \
	src/epub/HtmlTextExtractor.cpp \
	src/platform/PlatformFactory.cpp \
	src/platform/sdl/SDLRenderer.cpp \
	src/platform/sdl/SDLInput.cpp \
	src/platform/sdl/SDLFileSystem.cpp \
	src/platform/sdl/SDLClock.cpp \
	src/platform/nextui/NextUIRenderer.cpp \
	src/platform/nextui/NextUIInput.cpp \
	src/platform/nextui/NextUIFileSystem.cpp \
	src/platform/nextui/NextUIBindings.cpp \
	src/platform/nextui/NextUIClock.cpp \
	src/text/SentenceSplitter.cpp \
	src/text/TextTyper.cpp \
	src/text/Utf8.cpp \
	src/txt/TxtCompiler.cpp \
	src/ui/SceneManager.cpp \
	src/ui/BookListScene.cpp \
	src/ui/ChapterScene.cpp \
	src/ui/LoadingScene.cpp \
	src/ui/SettingsScene.cpp \
	src/ui/ReaderScene.cpp \
	src/ui/widgets/DialogueBox.cpp

TARGET := nextreading

all: $(TARGET)

tg5040:
	$(MAKE) -C ports/tg5040 ROOT_DIR=../..

export-tg5040:
	$(MAKE) -C ports/tg5040 ROOT_DIR=../.. export

list-platforms:
	@echo SDL desktop:
	@echo "  make"
	@echo TG5040 \(TrimUI Brick / Smart Pro\):
	@echo "  make tg5040"
	@echo "  make export-tg5040"
	@echo "  make -f ports/tg5040/Makefile.docker tg5040"
	@echo "  make -f ports/tg5040/Makefile.docker export"

pak: $(TARGET)
	mkdir -p $(PAK_DIR)
	mkdir -p $(PAK_ASSETS_DIR)
	cp $(TARGET) $(PAK_DIR)/$(TARGET)
	cp pak/launch.sh $(PAK_DIR)/launch.sh
	cp pak/manifest.json $(PAK_DIR)/manifest.json
	if [ -d pak/assets ]; then cp -R pak/assets/. $(PAK_ASSETS_DIR)/; fi

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(SDL_LIBS) $(ZIP_LIBS)

clean:
	rm -rf $(PAK_DIR)
	del /Q $(TARGET).exe 2>NUL || rm -f $(TARGET)
