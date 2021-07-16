
INCLUDE_FILES = ../include

CURR_SOURCES = $(wildcard *.c)
CURR_OBJECTS = $(patsubst %.c, ../obj/%.o, $(CURR_SOURCES))

CURR_DIR := $(shell pwd)
UPPER_DIR:=$(subst /qe, /qe,$(CURR_DIR))
$(warning $(UPPER_DIR))
AWOKE_DIR:=$(subst /application, /application,$(CURR_DIR))
AWOKE_DIR:=$(word 1, $(AWOKE_DIR))

INCLUDE_PATHS := -I$(LIB_PATH)/libutil/include \
				 -I$(LIB_PATH)/libevent/include \
				 -I$(LIB_PATH)/librtmsg/include \
                 -I$(LIB_PATH)/libpattern/include \
				 -I$(LIB_PATH)/libsecurity/include \
				 -I$(LIB_PATH)/libvinparser/include \
				 -I$(LIB_PATH)/libhttp/include \
				 -I$(CURR_DIR)/../

include $(AWOKE_DIR)/make.common

all: $(CURR_OBJECTS)
$(CURR_OBJECTS): ../obj/%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -I$(INCLUDE_FILES) -c $< -o $@
	
