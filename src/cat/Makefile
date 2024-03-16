CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

SOURCE = main.c
BUILD_PATH = ./s21_cat


all:
	$(CC) $(SOURCE) $(CFLAGS) -o $(BUILD_PATH)

clean:
	rm $(BUILD_PATH)