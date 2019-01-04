CC=gcc
CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc test/test.c test/head.o
	./kcc -test

	@./kcc "$$(gcc -E -P test/test.c)" > tmp-test.s
	@echo 'int global_arr[1] = {5}; int plus(int a, int b){return a+b;}' | gcc -xc -c -o tmp-test2.o -
	@gcc -o tmp-test tmp-test.s tmp-test2.o test/head.o
	@./tmp-test

clean:
	rm -f kcc *.o *~ tmp* test/head.o