.POSIX:

include config.mk

SRC = $(shell find . -name '*.c' | grep -v test_*.c | grep -v example.c)
OBJ = ${SRC:.c=.o}

EXTRAFLAGS =

# default to error
all: linear_hash

%.o: %.c
	@echo COMPILING CC $< with extra flags \"${EXTRAFLAGS}\"
	@${CC} -g -c ${CFLAGS} -DLH_TEST $< ${EXTRAFLAGS} -o $@

linear_hash: ${OBJ}
	@echo "ERROR: unable to compile linear_hash on it's own as it is a library"
	exit 1

cleanobj:
	@echo cleaning objects and temporary files
	@find . -iname '*.o' -delete

clean: cleanobj
	@echo cleaning tests
	@rm -f test_lh
	@echo cleaning gcov guff
	@find . -iname '*.gcda' -delete
	@find . -iname '*.gcov' -delete
	@find . -iname '*.gcno' -delete


test: run_tests

run_tests: compile_tests
	@echo "\n\nrunning test_lh"
	./test_lh
	@echo "\n"

compile_tests: clean ${OBJ}
	@echo "compiling tests"
	@${CC} test_linear_hash.c -o test_lh ${LDFLAGS} ${OBJ}
	@make -s cleanobj

.PHONY: all clean cleanobj linear_hash test
