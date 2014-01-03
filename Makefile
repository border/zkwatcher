UNAME := $(shell uname)

ZKPATH := ./libs/zookeeper

ifeq ($(UNAME), Darwin) 
	# For mac
	ZKPATH := ./libs/maclib/zookeeper
endif

all:
	gcc -g -std=gnu99 -o zkwatcher zkwatcher.c -DTHREADED -I$(ZKPATH)/include -L$(ZKPATH)/lib -lzookeeper_mt

clean:
	rm zkwatcher
