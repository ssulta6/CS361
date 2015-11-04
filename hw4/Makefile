SHELL := /bin/bash

TARGETS=homework4

CFLAGS=-Wall -g -O0

all: $(TARGETS)

homework4: homework4.c
	gcc $(CFLAGS) -o homework4 homework4.c

clean:
	rm -f $(TARGETS)

test-myserver: clean-test homework4
	source test-myserver.bash

test-pythonserver: clean-test
	source test-pythonserver.bash

test-diff: test-myserver test-pythonserver
	diff myserver.html pythonserver.html

clean-test: 
	rm -f myserver.html pythonserver.html

test: clean-test test-myserver test-pythonserver test-diff 
