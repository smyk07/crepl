CC = clang
CFLAGS = -std=c11 -g -Wall -Werror -Wextra -lm -ffast-math

OBJ_DIR = ./obj

SRCS = crepl.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

TARGET = crepl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean-all:
	rm -rf $(OBJ_DIR) $(TARGET)

clean-obj: 
	rm -rf $(OBJ_DIR)

clean: clean-all

.PHONY: all clean clean-all clean-obj
