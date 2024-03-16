CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

PATH_TESTS = ./tests/
BUILD = s21_grep

all: s21_grep

s21_grep: main.o
	$(CC) $(CFLAGS) main.o -o $(BUILD)

main.o:
	$(CC) $(CFLAGS) -c main.c -o main.o

.PHONY: tests

tests:
	cp ./tests/functional_test_s21_grep.sh ./
	./functional_test_s21_grep.sh
	rm -rf ./functional_test_s21_grep.sh

rebuild: clean all

clean:
	rm -rf *.{o,a,exe,so}
	rm -rf $(BUILD) 
	rm -rf ./logs

clang:
	find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \; 
	find . -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -n {} \; 