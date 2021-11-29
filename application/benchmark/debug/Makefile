
BENCHMAKR_ROOT:=$(subst /benchmark,/benchmark ,$(shell pwd))
BENCHMAKR_ROOT:=$(word 1, $(BENCHMAKR_ROOT))
OBJ_DIR := $(BENCHMAKR_ROOT)/obj
$(warning $(BENCHMAKR_ROOT))

INCLUDE_FILES = $(BENCHMAKR_ROOT)/include

SOUCE_FILES = $(wildcard *.c)
OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOUCE_FILES))

CURR_DIR := $(shell pwd)
AWOKE_DIR:=$(subst /application, /application,$(CURR_DIR))
AWOKE_DIR:=$(word 1, $(AWOKE_DIR))

INCLUDE_PATHS := -I$(LIB_PATH)/libutil/include \
				 -I$(LIB_PATH)/libevent/include \
				 -I$(LIB_PATH)/librtmsg/include \
                 -I$(LIB_PATH)/libpattern/include \
				 -I$(LIB_PATH)/libsecurity/include \
				 -I$(LIB_PATH)/libvinparser/include \
				 -I$(LIB_PATH)/libhttp/include \
				 -I$(BENCHMAKR_ROOT)/qe \
				 -I$(CURR_DIR)/../
				 
include $(AWOKE_DIR)/make.common

all: $(OBJ_FILES)
$(OBJ_FILES):$(OBJ_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -I$(INCLUDE_FILES) -c $< -o $@
