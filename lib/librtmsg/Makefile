
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /lib, /lib,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

LIB_NAME = librtmsg

LIB_OBJ_PATH := $(LIB_PATH)/$(LIB_NAME)/obj
LIB_LIB_PATH := $(LIB_PATH)/$(LIB_NAME)/lib
LIB_SRC_PATH := $(LIB_PATH)/$(LIB_NAME)/src
LIB_INC_PATH := $(LIB_PATH)/$(LIB_NAME)/include

LIB = $(LIB_LIB_PATH)/$(LIB_NAME).so
LIBS = -lutil

SRC_FILES= $(wildcard $(LIB_SRC_PATH)/*.c)
SRC_FILES_NODIR= $(notdir $(SRC_FILES))
OBJ_FILES= $(patsubst %.c,$(LIB_OBJ_PATH)/%.o,$(SRC_FILES_NODIR))

INCLUDE_PATHS := -I.\
				 -I$(LIB_INC_PATH) \
                 -I$(LIB_PATH)/libutil/include

all: prepare compile build install


.PHONY: prepare
prepare:
	@echo "-------------prepare--------------"
	if [ ! -d ${LIB_OBJ_PATH} ];then mkdir obj;fi;
	if [ ! -d ${LIB_LIB_PATH} ];then mkdir lib;fi;

.PHONY: compile
compile: $(OBJ_FILES)
$(OBJ_FILES): $(LIB_OBJ_PATH)/%.o : $(LIB_SRC_PATH)/%.c
	@echo "-------------compile--------------"
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(INC_PATH) -fPIC -c $< -o $@

.PHONY: build
build:$(LIB)
$(LIB): $(OBJ_FILES)
	@echo "-------------build--------------"
	$(CC) -shared -fPIC -Wl,--whole-archive,-soname,$@ $(CFLAGS) -o $@ $(OBJ_FILES) -Wl,--no-whole-archive $(LIBS)
	

.PHONY: install
install:
	@echo "-------------install--------------"
	cp $(LIB) $(LIB_PATH)

.PHONY: clean
clean:
	rm $(OBJ_FILES) $(LIB) $(LIB_PATH)/$(LIB_NAME).so