all:
	gcc -g -std=gnu99 -o zkwatcher zkwatcher.c -DTHREADED -I./libs/zookeeper/include -L./libs/zookeeper/lib -lzookeeper_mt

clean:
	rm zkwatcher
