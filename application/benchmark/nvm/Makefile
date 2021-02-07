
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(patsubst %.c, ../obj/%.o, $(SRC_FILES))

INCLUDE_FILES = ../include


all: $(OBJ_FILES)
$(OBJ_FILES):../obj/%.o:%.c
../obj/%.o:%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_FILES) -c $< -o $@