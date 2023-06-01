CC = gcc

SOURCE_DIR = src
INCLUDE_DIR = include
OBJECT_DIR = obj

_create_object_dir := $(shell mkdir -p $(OBJECT_DIR))

CFLAGS = -I$(INCLUDE_DIR) -c -fPIC -fstack-protector-all \
	-Wstrict-overflow -Wformat=2 -Wformat-security -Wall -Wextra \
	-g3 -O3 -Werror
LFLAGS = -shared

SOURCE_FILES = $(SOURCE_DIR)/chashmap.c
HEADER_FILES = $(INCLUDE_DIR)/chashmap.h
OBJ_FILES = $(SOURCE_FILES:$(SOURCE_DIR)/%.c=$(OBJECT_DIR)/%.o)

default: all

all: libchashmap.so

libchashmap.so: $(OBJ_FILES)
	$(CC) -o libchashmap.so $(OBJ_FILES) $(LFLAGS)

$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADER_FILES)
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -rf libchashmap.so $(OBJECT_DIR) test/tests test/coverage
