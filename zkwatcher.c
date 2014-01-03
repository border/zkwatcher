#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include "zookeeper/zookeeper.h"

static int
add_children_watch_on(zhandle_t *zh, const char *path, watcher_fn watcher, void *watcherCtx) 
{
    int ret = 0;

    struct String_vector strings;
    struct Stat stat;

    printf("add_children_watch_on path: %s\n", path);

    ret = zoo_wget_children2(zh, path, watcher, watcherCtx, &strings, &stat);
    if (ret) {
        fprintf(stderr, "zoo_wget_children2 error [%d]\n", ret);
    }
    return ret;
}

    static int
cmpid(const void *p1, const void *p2)
{
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

void zooEvent(int type) {
    if (type == ZOO_CREATED_EVENT) {
        printf("zoo create event, type[%d]\n", type);
    } else if (type == ZOO_DELETED_EVENT) {
        printf("zoo deleted event, type[%d]\n", type);
    } else if (type == ZOO_CHANGED_EVENT) {
        printf("zoo changed event, type[%d]\n", type);
    } else if (type == ZOO_CHILD_EVENT) {
        printf("zoo child event, type[%d]\n", type);
    } else if (type == ZOO_SESSION_EVENT) {
        printf("zoo session event, type[%d]\n", type);
    } else if (type == ZOO_NOTWATCHING_EVENT) {
        printf("zoo notwatching event, type[%d]\n", type);
    }
}

    void 
ccs_children_watcher(zhandle_t* zh, int type, int state,
        const char* path, void* watcherCtx)
{

    printf("child event happened: type[%d], watcherCtx: %s\n", type, (char *)watcherCtx);

    zooEvent(type);

    /*
       struct Stat {
       int64_t czxid;
       int64_t mzxid;
       int64_t ctime; //use this
       int64_t mtime;
       int32_t version;
       int32_t cversion;
       int32_t aversion;
       int64_t ephemeralOwner;
       int32_t dataLength;
       int32_t numChildren;
       int64_t pzxid;
       };
       */

    int ret = 0;
    struct String_vector strings;
    struct Stat stat;
    ret = zoo_wget_children2(zh, "/zkwatcher", ccs_children_watcher, watcherCtx, &strings, &stat);
    if (ret) {
        fprintf(stderr, "child: zoo_wget_children2 error [%d]\n", ret);
        return;
    }

    if (strings.count == 0) return;

    /* only care child event */
    //if (type != ZOO_CHILD_EVENT) return;

    /* routine for item creating */
    char* *p = NULL;
    p = malloc(strings.count * sizeof(char*));
    if (p == NULL) {
        fprintf(stderr, "child: malloc error\n");
        return;
    }
    memset(p, 0, strings.count * sizeof(char*));

    for (int i = 0;  i < strings.count; i++) {
        p[i] = strings.data[i];
    }
    qsort(&p[0], strings.count, sizeof(p[0]), cmpid);

    /* 
    for (int i = 0;  i < strings.count; i++) {
        puts(p[i]);
        printf("puts %d:%s\n", i, p[i]);
    }
    */

    for (int i = 0;  i < strings.count; i++) {
        char childpath[128] = {0};
        char value[128] = {0};
        int value_len = sizeof(value);
        struct Stat item_stat;
        sprintf(childpath, "/zkwatcher/%s", p[i]);

        ret = zoo_wget(zh, childpath, ccs_children_watcher, childpath, value, &value_len, &stat);
        if (ret) {
            printf("zoo_wget %s error\n", childpath);
        } else {
            printf("zoo_wget %s:%s\n", childpath, value);
        }
    }
    free(p);
    return;
}

int create_root(zhandle_t *zkhandle, const char *node_name, const char *data) {
    int ret;
    struct Stat stat;
    char node[512] = {0};

    printf("zoo_create /zkwatcher node\n");

    ret = zoo_exists(zkhandle, node_name, true, &stat);

    if (ret == ZOK) {
        printf("create_root %s already create\n", node_name);
    } else if (ret == ZNONODE){
        ret = zoo_create(zkhandle,
                node_name, 
                data, 
                sizeof(data),
                &ZOO_OPEN_ACL_UNSAFE,  /* a completely open ACL */
                0,
                node,
                sizeof(node)-1);
        if (ret) {
            fprintf(stderr, "zoo_create error [%d]\n", ret);
        }
    } else {
        printf("create_root error\n"); 
    }
    return ret;
}

    int 
main(int argc, const char *argv[])
{
    const char* host = "127.0.0.1:9001, 127.0.0.1:9002, 127.0.0.1:9003";
    zhandle_t* zkhandle;
    int timeout = 5000;
    int ret = 0;

    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    zkhandle = zookeeper_init(host, NULL, timeout, 
            0, "Zookeeper examples: config center services", 0);
    if (zkhandle == NULL) {
        fprintf(stderr, "Connecting to zookeeper servers error...\n");
        exit(EXIT_FAILURE);
    }

    create_root(zkhandle, "/zkwatcher", "hello");

    printf("Start watcher /zkwatcher children node\n");
    ret = add_children_watch_on(zkhandle, "/zkwatcher", ccs_children_watcher, "/zkwatcher");
    if (ret) {
        fprintf(stderr, "add_children_watch_on error [%d]\n", ret);
        exit(EXIT_FAILURE);
    }

    sleep(50000); // only for experiments

    zookeeper_close(zkhandle);
}

