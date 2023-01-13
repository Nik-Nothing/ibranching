PREFIX := ${HOME}

CC := gcc
STRIP := strip

CFLAGS := -O0 -std=gnu17 -Wall -g
LDFLAGS :=

STRIPFLAGS := --strip-all -R .comment -x

all:
	rm ib
	ib ib.c.ib -in --flags "$(CFLAGS) $(LDFLAGS)"

install:
	cp ib ${PREFIX}/bin/

test: all
	./ib ib.c.ib -in --flags "$(CFLAGS) $(LDFLAGS)"
	make -C testing

sa:
	cppcheck cppcheck --enable=all --inconclusive --library=posix --force --suppress=missingIncludeSystem ib.c
	frama-c ib.c -c11

clean:
	rm ib

bootstrap:
	$(CC) ib.c -o ib $(CFLAGS) $(LDFLAGS)
	cp ib ${PREFIX}/bin/
