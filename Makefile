.RECIPEPREFIX := >
PROJECT_NAME := game

INC_DIR := inc
SRC_DIR := src
CFLAGS := -no-pie -g -m32 -znoexecstack
NASM_FLAGS := -f elf -F dwarf -g
NASM_FLAGS_32 := -f elf32 -d ELF_TYPE -g -F dwarf

TARGET := $(PROJECT_NAME).out
OBJS = asm_io.o driver.o $(PROJECT_NAME).o 

.PHONY: all clean test

all: $(TARGET)

clean:
>rm -rf $(TARGET) $(OBJS)

$(TARGET): $(OBJS) $(INC_DIR)/$(PROJECT_NAME).h $(SRC_DIR)/$(PROJECT_NAME).c
>gcc $(CFLAGS) $^ -I $(INC_DIR) -o $@

$(PROJECT_NAME).o: $(SRC_DIR)/$(PROJECT_NAME).asm $(INC_DIR)/asm_io.inc
>nasm $(NASM_FLAGS) $< -I $(INC_DIR) -o $@

driver.o: $(SRC_DIR)/driver.c $(INC_DIR)/cdecl.h
>gcc $(CFLAGS) -c $< -I $(INC_DIR)

asm_io.o: $(SRC_DIR)/asm_io.asm
>nasm $(NASM_FLAGS_32) $< -I $(INC_DIR) -o $@

test: $(TARGET)
>./$(TARGET) --test