CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g
TARGET  = statimg

.PHONY: all clean test test-valgrind

all: $(TARGET)

$(TARGET): statimg.c vsfs.h
	$(CC) $(CFLAGS) -o $(TARGET) statimg.c

test: $(TARGET)
	./$(TARGET) test_imgs/clean.img
	@echo "--- Expect corrupt on leak_block.img ---"
	-./$(TARGET) test_imgs/leak_block.img
	@echo "--- Expect corrupt on unmarked_block.img ---"
	-./$(TARGET) test_imgs/unmarked_block.img
	@echo "--- Expect corrupt on dup_block.img ---"
	-./$(TARGET) test_imgs/dup_block.img
	@echo "--- Expect corrupt on dir_links.img ---"
	-./$(TARGET) test_imgs/dir_links.img
	@echo "--- Expect corrupt on orphan.img ---"
	-./$(TARGET) test_imgs/orphan.img

test-valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 \
		./$(TARGET) test_imgs/clean.img

clean:
	rm -f $(TARGET)
