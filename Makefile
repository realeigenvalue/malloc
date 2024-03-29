# not using standard Makefile template because this makefile creates shared
# objects and weird stuff
CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
CFLAGS_DEBUG   = -O0 $(WARNINGS) -g -std=c99 -D_GNU_SOURCE
CFLAGS_RELEASE = -O3 $(WARNINGS) -g -std=c99 -D_GNU_SOURCE

INC = -I.
TESTERS = $(patsubst %.c, %, $(wildcard testers/*))

all: alloc.so contest-alloc.so mreplace mcontest $(TESTERS:testers/%=testers_exe/%)

alloc.so: alloc.c
	$(CC) $^ $(CFLAGS_DEBUG) -o $@ -shared -fPIC

contest-alloc.so: contest-alloc.c
	$(CC) $^ $(CFLAGS_RELEASE) -o $@ -shared -fPIC -ldl

mreplace: mreplace.c
	$(CC) $^ $(CFLAGS_RELEASE) -o $@

mcontest: mcontest.c contest.h
	$(CC) $< $(CFLAGS_RELEASE) -o $@ -ldl -lpthread

# testers compiled in debug mode to prevent compiler from optimizing away the
# behavior we are trying to test
testers_exe/%: testers/%.c
	@mkdir -p testers_exe/
	$(CC) $^ $(CFLAGS_DEBUG) -o $@

.PHONY : clean
clean:
	-rm -rf *.o *.so mreplace mcontest testers_exe/