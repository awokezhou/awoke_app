$(warning $(LIB_PATH))

LIB_OBJ_PATH := $(LIB_PATH)/obj

LIB = libawoke.so

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, $(LIB_OBJ_PATH)/%.o, $(SRC_FILES))


all: prepare compile build install


.PHONY: prepare
prepare:
	echo "-------------prepare--------------"
	if [ ! -d ${LIB_OBJ_PATH} ];then mkdir obj;fi;


.PHONY: compile
compile: $(OBJ_FILES)
$(OBJ_FILES): $(LIB_OBJ_PATH)/%.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

.PHONY: build
build:$(LIB)
$(LIB): $(OBJ_FILES)
	echo "-------------build--------------"
	$(CC) -shared -fPIC -Wl,--whole-archive,-soname,$@ $(CFLAGS) -o $@ $(OBJ_FILES) -Wl,--no-whole-archive
	

.PHONY: install
install:
	echo "-------------install--------------"

.PHONY: clean
clean:
	rm $(OBJ_FILES) $(LIB)
