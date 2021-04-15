
INCLUDE_FILES = ../include

CURR_SOURCES = $(wildcard *.c)
CURR_OBJECTS = $(patsubst %.c, ../obj/%.o, $(CURR_SOURCES))

CURR_DIR := $(shell pwd)
AWOKE_DIR:=$(subst /application, /application,$(CURR_DIR))
AWOKE_DIR:=$(word 1, $(AWOKE_DIR))

include $(AWOKE_DIR)/make.common

all: $(CURR_OBJECTS)
$(CURR_OBJECTS): ../obj/%.o:%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_FILES) -c $< -o $@