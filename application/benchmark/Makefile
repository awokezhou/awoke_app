
EXEC = bin/benchmark
LIBS = -lpthread -lutil -lsecurity -levent -lrtmsg -lpattern -lvinparser -lhttp

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, obj/%.o, $(SRC_FILES))

SUB_DIRS := nvm sync qe

INCLUDE_FILES = include

CURR_DIR := $(shell pwd)
AWOKE_DIR:=$(subst /application, /application,$(CURR_DIR))
AWOKE_DIR:=$(word 1, $(AWOKE_DIR))

include $(AWOKE_DIR)/make.common

INCLUDE_PATHS := -I$(LIB_PATH)/libutil/include \
                 -I$(LIB_PATH)/libevent/include \
                 -I$(LIB_PATH)/librtmsg/include \
                 -I$(LIB_PATH)/libpattern/include \
				 -I$(LIB_PATH)/libsecurity/include \
				 -I$(LIB_PATH)/libvinparser/include \
				 -I$(LIB_PATH)/libhttp/include \
				 -I$(CURR_DIR)/nvm \
				 -I$(CURR_DIR)/sync \
				 -I$(CURR_DIR)/qe \
				 -I$(CURR_DIR)/include

all: prepare compile build


.PHONY: prepare
prepare:
	if [ ! -d "bin" ];then mkdir bin;fi;
	if [ ! -d "obj" ];then mkdir obj;fi;

.PHONY: compile
compile:$(SUB_DIRS) $(OBJ_FILES)

.PHONY: $(SUB_DIRS)
$(SUB_DIRS):
	$(MAKE) -C $@

$(OBJ_FILES): obj/%.o:%.c
obj/%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

.PHONY: build
build:$(EXEC)
$(EXEC):
	$(CC) $(CFLAGS) -T ldscript.lds -o $@ obj/*.o -Wl,-rpath $(LIB_PATH) -L$(LIB_PATH) -L/usr/lib/x86_64-linux-gnu $(LIBS) -lm
	
.PHONY: clean
clean:
	rm $(OBJ_FILES) $(EXEC) obj/*.o
