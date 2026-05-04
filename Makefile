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
PROD ?= true

OBJS := asm_io.o driver.o $(PROJECT_FILE_PREFIX).o
PKGS := nasm gcc make gcc-multilib libc6-dev-i386 lib32gcc-s1 lib32ncurses-dev libasound2t64:i386 libpulse0:i386
LIBS := -L$(LIB_DIR) -l:libncursesw.so.6 -l:libtinfo.so.6 -lpthread -lm -ldl

ifeq ($(PROD),true)
  TARGET := $(PROJECT_NAME)
else
  TARGET := $(PROJECT_FILE_PREFIX).out
endif

.PHONY: all install clean test submission

all: $(TARGET)

install:
>sudo dpkg --add-architecture i386
>sudo apt-get update
>sudo apt-get install -y $(PKGS)

clean:
>rm -rf $(PROJECT_NAME) $(PROJECT_FILE_PREFIX).out $(OBJS)

test: $(TARGET)
>test -f ./$(TARGET) || { echo "error: $(TARGET) was not built"; exit 1; }
>test -x ./$(TARGET) || { echo "error: $(TARGET) is not executable"; exit 1; }
>binary_info="$$(file ./$(TARGET))"; \
>echo "$$binary_info"; \
>echo "$$binary_info" | grep -q 'ELF 32-bit' || { echo "error: $(TARGET) is not a 32-bit ELF binary"; exit 1; }; \
>echo "$$binary_info" | grep -q 'dynamically linked' || { echo "error: $(TARGET) is not dynamically linked"; exit 1; }
>ldd ./$(TARGET) | tee /tmp/$(TARGET).ldd
>! grep -q 'not found' /tmp/$(TARGET).ldd || { echo "error: one or more runtime libraries are missing"; exit 1; }
>TERM=xterm ./$(TARGET) test

submission: $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) -DDISABLE_AUDIO $^ -I $(INC_DIR) $(LIBS) $(RPATH) -o $(PROJECT_NAME)
>cp src/game.asm danger-dash.txt
>zip -r danger-dash.zip danger-dash.txt readme.txt Makefile src/ inc/ screenshots/ assets/

$(PROJECT_FILE_PREFIX).out: $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) $(LIBS) -o $@

$(PROJECT_NAME): $(OBJS) $(SRC_DIR)/$(PROJECT_FILE_PREFIX).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) $(LIBS) -o $@

$(PROJECT_FILE_PREFIX).o: $(SRC_DIR)/$(PROJECT_FILE_PREFIX).asm $(INC_DIR)/asm_io.inc
>nasm $(NASM_FLAGS) $< -I $(INC_DIR) -o $@

driver.o: $(SRC_DIR)/driver.c
>gcc $(CFLAGS) -c $< -I $(INC_DIR)

asm_io.o: $(SRC_DIR)/asm_io.asm
>nasm $(NASM_FLAGS_32) $< -I $(INC_DIR) -o $@