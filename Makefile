CC ?= cc
CFLAGS ?= -O3
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror
.PHONY: all test bench clean
all: utf8check
utf8check: main.o utf8.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
utf8test: test.o utf8.o reference.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
utf8bench: bench.o utf8.o reference.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
main.o test.o bench.o reference.o: utf8.h
test: utf8test
	./utf8test
bench: utf8bench
	./utf8bench
clean:
	rm -f *.o utf8check utf8test utf8bench
