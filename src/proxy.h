/*
 * Copyright (C) 2019  Giuseppe Fabio Nicotra <artix2 at gmail dot com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __REDIS_CLUSTER_PROXY_H__
#define __REDIS_CLUSTER_PROXY_H__

#include <pthread.h>
#include <stdint.h>
#include <sys/resource.h>
#include <hiredis.h>
#include <time.h>
#include "ae.h"
#include "anet.h"
#include "cluster.h"
#include "commands.h"
#include "sds.h"
#include "rax.h"

#define REDIS_CLUSTER_PROXY_VERSION "0.0.1"
#define CLIENT_STATUS_NONE          0
#define CLIENT_STATUS_LINKED        1
#define CLIENT_STATUS_UNLINKED      2

#define getClientLoop(c) (proxy.threads[c->thread_id]->loop)

struct client;
struct proxyThread;

typedef struct clientRequest{
    struct client *client;
    uint64_t id;
    sds buffer;
    int query_offset;
    int is_multibulk;
    int argc;
    // 当前请求包含的命令个数
    int num_commands;
    // 当前命令包含的字段个数
    long long pending_bulks;
    int current_bulk_length;
    // 命令每个字段在buffer的起始偏移量
    int *offsets;
    // 命令每个字段的长度
    int *lengths;
    // 字段个数
    int offsets_size;
    int slot;
    clusterNode *node;
    struct redisCommandDef *command;
    size_t written;
    int parsing_status;
    int has_write_handler;
} clientRequest;

typedef struct {
    // 主线程eventloop
    aeEventLoop *main_loop;
    // 服务端socket句柄
    int fds[2];
    int fd_count;
    int tcp_backlog;
    char neterr[ANET_ERR_LEN];
    // 工作线程数组
    struct proxyThread **threads;
    // 客户端连接数
    _Atomic uint64_t numclients;
    // 命令列表
    rax *commands;
    int min_reserved_fds;
    // 启动时间
    time_t start_time;
    sds configfile;
} redisClusterProxy;

typedef struct client {
    uint64_t id;
    int fd;
    sds ip;
    int thread_id;
    sds obuf;
    size_t written;
    list *reply_array;
    int status;
    int has_write_handler;
    uint64_t next_request_id;
    struct clientRequest *current_request; /* Currently reading */
    uint64_t min_reply_id;
    rax *unordered_replies;
    list *requests_to_process;       /* Requests not completely parsed */
    int requests_with_write_handler; /* Number of request that are still
                                      * being writing to cluster */
} client;

void freeRequest(clientRequest *req);
void freeRequestList(list *request_list);
void onClusterNodeDisconnection(clusterNode *node);

#endif /* __REDIS_CLUSTER_PROXY_H__ */
