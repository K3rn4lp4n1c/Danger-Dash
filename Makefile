.RECIPEPREFIX := >
PROJECT_NAME := danger-dash
PROJECT_FILE_PREFIX := game

INC_DIR := inc
SRC_DIR := src

CFLAGS := -no-pie -g -m32 -znoexecstack
NASM_FLAGS := -f elf -F dwarf -g
NASM_FLAGS_32 := -f elf32 -d ELF_TYPE -g -F dwarf

TARGET := $(PROJECT_FILE_PREFIX).out
STATIC_TARGET := $(PROJECT_NAME)

NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
NCURSES_STATIC_LIBS := $(shell pkg-config --libs --static ncursesw)

OBJS := asm_io.o driver.o $(PROJECT_FILE_PREFIX).o
PKGS := nasm gcc make gcc-multilib libc6-dev-i386 lib32gcc-s1 lib32ncurses-dev libasound2t64:i386 libpulse0:i386
LIBS := $(NCURSES_LIBS) -lpthread -lm -ldl
STATIC_LIBS := $(NCURSES_STATIC_LIBS) -lpthread -lm -ldl

.PHONY: all build install clean test examine

all: $(TARGET)

build: $(STATIC_TARGET)

install:
>sudo apt-get update
>sudo apt-get install -y $(PKGS)

clean:
>rm -rf $(TARGET) $(STATIC_TARGET) $(OBJS)

$(TARGET): $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) $(LIBS) -o $@

$(STATIC_TARGET): $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) -static $(STATIC_LIBS) -o $@

$(PROJECT_FILE_PREFIX).o: $(SRC_DIR)/$(PROJECT_FILE_PREFIX).asm $(INC_DIR)/asm_io.inc
>nasm $(NASM_FLAGS) $< -I $(INC_DIR) -o $@

driver.o: $(SRC_DIR)/driver.c
>gcc $(CFLAGS) -c $< -I $(INC_DIR)

asm_io.o: $(SRC_DIR)/asm_io.asm
>nasm $(NASM_FLAGS_32) $< -I $(INC_DIR) -o $@

test: $(TARGET)
>./$(TARGET) test

examine: $(STATIC_TARGET)
>./$(STATIC_TARGET) test