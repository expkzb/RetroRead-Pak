ROOT_DIR ?= ../..
TARGET := nextreading
APP_NAME := RetroRead
BUILD_DIR := build
TARGET_PATH := $(BUILD_DIR)/$(TARGET)
EXPORT_DIR := $(ROOT_DIR)/dist/tg5040/$(APP_NAME).pak

ifeq ($(origin CROSS_COMPILE), undefined)
CROSS_COMPILE := arm-linux-gnueabihf-
endif

ifeq ($(origin CXX), default)
CXX := $(CROSS_COMPILE)g++
endif

ifeq ($(origin STRIP), default)
STRIP := $(CROSS_COMPILE)strip
endif

ifeq ($(origin PKG_CONFIG), default)
PKG_CONFIG := pkg-config
endif
ifeq ($(strip $(PKG_CONFIG)),)
PKG_CONFIG := pkg-config
endif

SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
ZIP_CFLAGS := $(shell $(PKG_CONFIG) --cflags libzip 2>/dev/null)
ZIP_LIBS := $(shell $(PKG_CONFIG) --libs libzip 2>/dev/null)
FREETYPE_CFLAGS := $(shell $(PKG_CONFIG) --cflags freetype2 2>/dev/null)
FREETYPE_LIBS := $(shell $(PKG_CONFIG) --libs freetype2 2>/dev/null)
FS_LIBS := -lstdc++fs

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic -DNEXTREADING_NO_SDL_TTF -DNEXTREADING_TG5040 -I$(ROOT_DIR)/src $(SDL_CFLAGS) $(ZIP_CFLAGS) $(FREETYPE_CFLAGS)

.PHONY: all strip export clean

SOURCES := \
	$(ROOT_DIR)/src/app/main.cpp \
	$(ROOT_DIR)/src/app/Application.cpp \
	$(ROOT_DIR)/src/audio/TextBlipPlayer.cpp \
	$(ROOT_DIR)/src/core/BookLibrary.cpp \
	$(ROOT_DIR)/src/core/BookCache.cpp \
	$(ROOT_DIR)/src/core/ProgressStore.cpp \
	$(ROOT_DIR)/src/core/SettingsStore.cpp \
	$(ROOT_DIR)/src/epub/EpubCompiler.cpp \
	$(ROOT_DIR)/src/epub/EpubArchive.cpp \
	$(ROOT_DIR)/src/epub/HtmlTextExtractor.cpp \
	$(ROOT_DIR)/src/platform/PlatformFactory.cpp \
	$(ROOT_DIR)/src/platform/sdl/SDLRenderer.cpp \
	$(ROOT_DIR)/src/platform/sdl/SDLInput.cpp \
	$(ROOT_DIR)/src/platform/sdl/SDLFileSystem.cpp \
	$(ROOT_DIR)/src/platform/sdl/SDLClock.cpp \
	$(ROOT_DIR)/src/platform/nextui/NextUIRenderer.cpp \
	$(ROOT_DIR)/src/platform/nextui/NextUIInput.cpp \
	$(ROOT_DIR)/src/platform/nextui/NextUIFileSystem.cpp \
	$(ROOT_DIR)/src/platform/nextui/NextUIBindings.cpp \
	$(ROOT_DIR)/src/platform/nextui/NextUIClock.cpp \
	$(ROOT_DIR)/src/text/SentenceSplitter.cpp \
	$(ROOT_DIR)/src/text/TextTyper.cpp \
	$(ROOT_DIR)/src/text/Utf8.cpp \
	$(ROOT_DIR)/src/txt/TxtCompiler.cpp \
	$(ROOT_DIR)/src/ui/SceneManager.cpp \
	$(ROOT_DIR)/src/ui/BookListScene.cpp \
	$(ROOT_DIR)/src/ui/ChapterScene.cpp \
	$(ROOT_DIR)/src/ui/LoadingScene.cpp \
	$(ROOT_DIR)/src/ui/SettingsScene.cpp \
	$(ROOT_DIR)/src/ui/ReaderScene.cpp \
	$(ROOT_DIR)/src/ui/widgets/DialogueBox.cpp

all: $(TARGET_PATH)

$(TARGET_PATH): $(SOURCES)
	@mkdir -p $(BUILD_DIR)
	@if ! command -v $(CXX) >/dev/null 2>&1; then \
		echo "Missing TG5040 cross compiler: $(CXX)"; \
		echo "Set CROSS_COMPILE or CXX to your TG5040 toolchain. See ports/tg5040/README.md."; \
		exit 1; \
	fi
	@if [ -z "$(SDL_LIBS)" ] || [ -z "$(ZIP_LIBS)" ]; then \
		echo "Missing TG5040 SDL/libzip pkg-config metadata."; \
		echo "Provide the device toolchain/sysroot environment first. See ports/tg5040/README.md."; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET_PATH) $(SDL_LIBS) $(ZIP_LIBS) $(FREETYPE_LIBS) $(FS_LIBS)

strip: $(TARGET_PATH)
	-$(STRIP) $(TARGET_PATH)

export: $(TARGET_PATH)
	mkdir -p $(EXPORT_DIR)/assets/fonts
	mkdir -p $(EXPORT_DIR)/lib
	cp $(TARGET_PATH) $(EXPORT_DIR)/$(TARGET)
	cp $(ROOT_DIR)/ports/tg5040/pak/launch.sh $(EXPORT_DIR)/launch.sh
	cp $(ROOT_DIR)/ports/tg5040/pak/pak.json $(EXPORT_DIR)/pak.json
	if [ -d $(ROOT_DIR)/pak/assets ]; then cp -R $(ROOT_DIR)/pak/assets/. $(EXPORT_DIR)/assets/; fi
	@set -e; \
	needed_libs=$$(readelf -d $(TARGET_PATH) | awk '/NEEDED/{gsub(/\[|\]/,"",$$5); print $$5}'); \
	search_roots="$(SYSROOT)/usr/lib $(SYSROOT)/lib /opt/aarch64-nextui-linux-gnu/lib /opt/aarch64-nextui-linux-gnu/aarch64-nextui-linux-gnu/lib"; \
	for lib in $$needed_libs; do \
		case "$$lib" in libc.so.6|libm.so.6|libpthread.so.0|libdl.so.2|librt.so.1|ld-linux-aarch64.so.1) continue ;; esac; \
		found=""; \
		for root in $$search_roots; do \
			if [ -f "$$root/$$lib" ]; then found="$$root/$$lib"; break; fi; \
		done; \
		if [ -z "$$found" ]; then \
			found=$$(find $(SYSROOT) /opt/aarch64-nextui-linux-gnu -type f -name "$$lib" 2>/dev/null | head -n1); \
		fi; \
		if [ -n "$$found" ]; then \
			cp -L "$$found" $(EXPORT_DIR)/lib/; \
		else \
			echo "warning: missing runtime library $$lib"; \
		fi; \
	done; \
	for extra in libz.so.1 libbz2.so.1.0 libpng.so.3 libharfbuzz.so.0 libglib-2.0.so.0; do \
		found=""; \
		for root in $$search_roots; do \
			if [ -f "$$root/$$extra" ]; then found="$$root/$$extra"; break; fi; \
		done; \
		if [ -n "$$found" ]; then cp -L "$$found" $(EXPORT_DIR)/lib/; fi; \
	done

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(EXPORT_DIR)
