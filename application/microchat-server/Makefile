
EXEC = bin/server
LIBS = -lpthread -lutil -lwebsockets -levent
SRC_FILES= $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, obj/%.o, $(SRC_FILES))

INCLUDE_FILES = include

CURR_DIR := $(shell pwd)
AWOKE_DIR:=$(subst /application, /application,$(CURR_DIR))
AWOKE_DIR:=$(word 1, $(AWOKE_DIR))

include $(AWOKE_DIR)/make.common

INCLUDE_PATHS := -I$(LIB_PATH)/libutil/include \
					-I$(LIB_PATH)/libevent/include

all: prepare compile build


.PHONY: prepare
prepare:
	if [ ! -d "bin" ];then mkdir bin;fi;
	if [ ! -d "obj" ];then mkdir obj;fi;


.PHONY: compile
compile:$(OBJ_FILES)
$(OBJ_FILES): obj/%.o:%.c
obj/%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -I$(INCLUDE_FILES) -c $< -o $@


.PHONY: build
build:$(EXEC)
$(EXEC):$(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ -Wl,-rpath $(LIB_PATH) -L$(LIB_PATH) -L/usr/lib/x86_64-linux-gnu $(LIBS) -lm

.PHONY: clean
clean:
	rm $(OBJ_FILES) $(EXEC)
