.RECIPEPREFIX := >
PROJECT_NAME := danger-dash
PROJECT_FILE_PREFIX := game

INC_DIR := inc
SRC_DIR := src
LIB_DIR := lib

CFLAGS := -no-pie -g -m32 -znoexecstack
RPATH := -Wl,-rpath,'$$ORIGIN/$(LIB_DIR)'
NASM_FLAGS := -f elf -F dwarf -g
NASM_FLAGS_32 := -f elf32 -d ELF_TYPE -g -F dwarf

TARGET := $(PROJECT_FILE_PREFIX).out

OBJS := asm_io.o driver.o $(PROJECT_FILE_PREFIX).o
PKGS := nasm gcc make gcc-multilib libc6-dev-i386 lib32gcc-s1 lib32ncurses-dev libasound2t64:i386 libpulse0:i386
LIBS := -lncursesw -ltinfo -lpthread -lm -ldl

.PHONY: all install clean test package

all: $(TARGET)
package: $(PROJECT_NAME)

install:
>sudo dpkg --add-architecture i386
>sudo apt-get update
>sudo apt-get install -y $(PKGS)

clean:
>rm -rf $(PROJECT_NAME) $(TARGET) $(STATIC_TARGET) $(OBJS)

test: $(TARGET)
>./$(TARGET) test

$(TARGET): $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) $(LIBS) -o $@

$(PROJECT_NAME): $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) $(RPATH) $(LIBS) -o $@

$(PROJECT_FILE_PREFIX).o: $(SRC_DIR)/$(PROJECT_FILE_PREFIX).asm $(INC_DIR)/asm_io.inc
>nasm $(NASM_FLAGS) $< -I $(INC_DIR) -o $@

driver.o: $(SRC_DIR)/driver.c
>gcc $(CFLAGS) -c $< -I $(INC_DIR)

asm_io.o: $(SRC_DIR)/asm_io.asm
>nasm $(NASM_FLAGS_32) $< -I $(INC_DIR) -o $@