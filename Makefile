# weird-player: build outputs go into build/
CXX      ?= g++
CXXFLAGS ?= -O2 -Wall
PKGS      =  libavformat libavcodec libavutil

SRC      := shappy_player.cpp
ifeq ($(OS),Windows_NT)
EXE      := .exe
else
EXE      :=
endif
TARGET   := build/shappy_player$(EXE)

PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS))
PKG_LIBS   := $(shell pkg-config --libs $(PKGS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(PKG_CFLAGS) -o $@ $< $(PKG_LIBS)

run: all
	./$(TARGET) test1.mp4

clean:
	rm -rf build
