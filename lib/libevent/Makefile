
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /lib, /lib,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

LIB_OBJ_PATH := $(LIB_PATH)/librtmsg/obj
LIB_LIB_PATH := $(LIB_PATH)/librtmsg/lib

LIB = $(LIB_LIB_PATH)/libevent.so
LIBS = -lutil

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, $(LIB_OBJ_PATH)/%.o, $(SRC_FILES))


all: prepare compile build install


.PHONY: prepare
prepare:
	echo "-------------prepare--------------"
	if [ ! -d ${LIB_OBJ_PATH} ];then mkdir obj;fi;
	if [ ! -d ${LIB_LIB_PATH} ];then mkdir obj;fi;


.PHONY: compile
compile: $(OBJ_FILES)
$(OBJ_FILES): $(LIB_OBJ_PATH)/%.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

.PHONY: build
build:$(LIB)
$(LIB): $(OBJ_FILES)
	echo "-------------build--------------"
	$(CC) -shared -fPIC -Wl,--whole-archive,-soname,$@ $(CFLAGS) -o $@ $(OBJ_FILES) -Wl,--no-whole-archive $(LIBS) -lm
	

.PHONY: install
install:
	echo "-------------install--------------"
	cp $(LIB) $(LIB_PATH)

.PHONY: clean
clean:
	rm $(OBJ_FILES) $(LIB) $(LIB_PATH)/libevent.so