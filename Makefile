CFLAGS = -std=c11 -Wall -g -static

SRCS = codegen.c container.c main.c parse.c
OBJS = $(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
