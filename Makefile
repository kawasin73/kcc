CC=gcc
CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./kcc -test
	./test.sh

clean:
	rm -f kcc *.o *~ tmp*