#include "csapp.h"
#include <bits/pthreadtypes.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_NUM 10

/* Multithreading options */
#define SBUFSIZE 16
#define NTHREADS 4

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct {
  int *buf;    /* Buffer array */
  int n;       /* Maximum number of slots */
  int front;   /* Index of the (first - 1) item */
  int rear;    /* Index of the last item */
  sem_t mutex; /* Protects accesses to buf */
  sem_t slots; /* Counts available slots */
  sem_t items; /* Counts available items */
} Sbuf;

typedef struct {
  char hostname[MAXLINE];
  char query[MAXLINE];
  char obj[MAX_OBJECT_SIZE];
  size_t obj_size;
  unsigned int time_point;
} Block;

typedef struct {
  Block data[CACHE_NUM];
  size_t data_size;
} Cache;

static Sbuf sbuf; /* Shared buffer of connected descriptors */
static Cache cache;
static sem_t mutex, w;
static int readcnt;
static unsigned int timestamp;

void doit(int fd);
int parse_uri(char *uri, char *hostname, char *query, char *port);
int send_request(int fd, char *hostname, char *query);
int send_back(int fd, rio_t *server_rio, char *obj, size_t *obj_size);
void *thread(void *vargp);
void sbuf_init(Sbuf *sp, int n);
void sbuf_deinit(Sbuf *sp);
void sbuf_insert(Sbuf *sp, int item);
int sbuf_remove(Sbuf *sp);
void init_cache();
int get_cache(int fd, char *hostname, char *query);
int add_cache(char *hostname, char *query, char *obj, size_t obj_size);

int main(int argc, char *argv[]) {
  int listenfd, connfd, rc, i;
  pthread_t tid;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command-line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Capture signal SIGPIPE */
  Signal(SIGPIPE, SIG_IGN);

  listenfd = Open_listenfd(argv[1]);

  sbuf_init(&sbuf, SBUFSIZE);
  for (i = 0; i < NTHREADS; i++) { /* Create worker threads */
    Pthread_create(&tid, NULL, thread, NULL);
  }

  init_cache();

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd < 0) {
      fprintf(stderr, "accept() error: %s\n", strerror(errno));
      continue;
    }
    rc = getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                     MAXLINE, 0);
    if (rc != 0) {
      fprintf(stderr, "getnameinfo() error: %s\n", gai_strerror(rc));
    } else {
      printf("Accepted connection from (%s, %s)\n", hostname, port);
    }
    sbuf_insert(&sbuf, connfd);
  }
  Close(listenfd);
}

/*
 * doit - Handle one HTTP request/response transaction
 */
void doit(int fd) {
  int server_fd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], query[MAXLINE], port[MAXLINE];
  char obj[MAX_OBJECT_SIZE];
  size_t obj_size;
  rio_t rio, server_rio;

  /* Read request line and headers */
  rio_readinitb(&rio, fd);
  if (rio_readlineb(&rio, buf, MAXLINE) <= 0) {
    fprintf(stderr, "rio_readlineb() error\n");
    return;
  }
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET")) {
    fprintf(stderr, "Proxy does not implement this method\n");
    return;
  }
  if (!parse_uri(uri, hostname, query, port)) {
    fprintf(stderr, "parse_uri() error\n");
    return;
  }

  /* Try to get cache from cached list */
  if (get_cache(fd, hostname, query)) {
    return;
  }

  /* Connect to server and send request */
  server_fd = open_clientfd(hostname, port);
  if (server_fd < 0) {
    fprintf(stderr, "open_clientfd() error\n");
    return;
  }
  if (!send_request(server_fd, hostname, query)) {
    fprintf(stderr, "send_request() error\n");
    close(server_fd);
    return;
  }

  /* Send back to client */
  rio_readinitb(&server_rio, server_fd);
  if (!send_back(fd, &server_rio, obj, &obj_size)) {
    fprintf(stderr, "send_back() error\n");
    close(server_fd);
    return;
  }

  /* Cache content received from server */
  if (obj_size <= MAX_OBJECT_SIZE) {
    add_cache(hostname, query, obj, obj_size);
  }

  if (close(server_fd) < 0) {
    fprintf(stderr, "close() error: %s\n", strerror(errno));
  }
}

/*
 * parse_uri - Parse uri
 */
int parse_uri(char *uri, char *hostname, char *query, char *port) {
  char *ptr = uri;
  char *start, *end;

  /* Uri should start with http */
  if (strstr(ptr, "http://") != ptr) {
    return 0;
  }
  ptr += 7;

  /* Get hostname and port (if required) */
  start = ptr;
  while (*ptr != '\0' && *ptr != ':' && *ptr != '/') {
    ptr++;
  }
  end = ptr;
  strncpy(hostname, start, end - start);
  hostname[end - start] = '\0';
  if (*ptr == ':') {
    ptr++;
    start = ptr;
    while (isdigit(*ptr)) {
      ptr++;
    }
    end = ptr;
    if (end > start) {
      strncpy(port, start, end - start);
      port[end - start] = '\0';
    } else {
      strcpy(port, "80");
    }
  } else {
    strcpy(port, "80");
  }

  /* Get path/query (if any) */
  if (*ptr == '/') {
    strcpy(query, ptr);
  } else {
    strcpy(query, "/");
  }
  return 1;
}

/*
 * send_request - Send request headers to server
 */
int send_request(int fd, char *hostname, char *query) {
  char buf[MAXBUF];

  sprintf(buf, "GET %s HTTP/1.0\r\n", query);
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  sprintf(buf, "Host: %s\r\n", hostname);
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  strcpy(buf, user_agent_hdr);
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  strcpy(buf, "Connection: close\r\n");
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  strcpy(buf, "Proxy-Connection: close\r\n");
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  strcpy(buf, "\r\n");
  if (rio_writen(fd, buf, strlen(buf)) <= 0) {
    return 0;
  }
  return 1;
}

/*
 * send_back - Send back response sent from server to the client
 */
int send_back(int fd, rio_t *server_rio, char *obj, size_t *obj_size) {
  ssize_t n;
  char buf[MAXBUF];
  char *ptr = obj;
  size_t size = 0;

  while ((n = rio_readlineb(server_rio, buf, MAXBUF)) > 0) {
    if (rio_writen(fd, buf, n) <= 0) {
      return 0;
    }
    if ((size + n) <= MAX_OBJECT_SIZE) {
      strncpy(ptr, buf, n);
      ptr += n;
    }
    size += n;
  }
  *obj_size = size;
  return 1;
}

/*
 * thread - Thread routine
 */
void *thread(void *vargp) {
  pthread_detach(pthread_self());
  while (1) {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    close(connfd);
  }
  return NULL;
}

/*
 * sbuf_init - Create an empty, bounded, shared FIFO buffer with n slots
 */
void sbuf_init(Sbuf *sp, int n) {
  sp->buf = calloc(n, sizeof(int));
  sp->n = n;
  sp->front = sp->rear = 0;
  sem_init(&sp->mutex, 0, 1);
  sem_init(&sp->slots, 0, n);
  sem_init(&sp->items, 0, 0);
}

/*
 * sbuf_deinit - Clean up buffer sp
 */
void sbuf_deinit(Sbuf *sp) { free(sp->buf); }

/*
 * sbuf_insert - Insert item onto the rear of shared buffer sp
 */
void sbuf_insert(Sbuf *sp, int item) {
  P(&sp->slots);
  P(&sp->mutex);
  sp->buf[(++sp->rear) % (sp->n)] = item;
  V(&sp->mutex);
  V(&sp->items);
}

/*
 * sbuf_remove - Remove and return the first item from buffer sp
 */
int sbuf_remove(Sbuf *sp) {
  int item;
  P(&sp->items);
  P(&sp->mutex);
  item = sp->buf[(++sp->front) % (sp->n)];
  V(&sp->mutex);
  V(&sp->slots);
  return item;
}

/*
 * init_cache - Init global cache params
 */
void init_cache() {
  timestamp = 0;
  readcnt = 0;
  sem_init(&mutex, 0, 1);
  sem_init(&w, 0, 1);
  cache.data_size = 0;
}

/*
 * get_cache - Get cache from cached list and transmit the object to fd
 */
int get_cache(int fd, char *hostname, char *query) {
  int hit_flag = 0;

  P(&mutex);
  readcnt++;
  if (readcnt == 1) {
    P(&w);
  }
  V(&mutex);

  for (int i = 0; i < cache.data_size; i++) {
    if (!strcmp(hostname, cache.data[i].hostname) &&
        !strcmp(query, cache.data[i].query)) {
      P(&mutex);
      cache.data[i].time_point = timestamp++;
      V(&mutex);
      rio_writen(fd, cache.data[i].obj, cache.data[i].obj_size);
      hit_flag = 1;
      break;
    }
  }

  P(&mutex);
  readcnt--;
  if (readcnt == 0) {
    V(&w);
  }
  V(&mutex);

  return hit_flag;
}

/*
 * add_cache - Add cache to caches list if any space remained
 */
int add_cache(char *hostname, char *query, char *obj, size_t obj_size) {
  size_t index;
  unsigned int tt;

  P(&w);
  if (cache.data_size == CACHE_NUM) {
    tt = timestamp;
    for (int i = 0; i < CACHE_NUM; i++) {
      if (cache.data[i].time_point < tt) {
        tt = cache.data[i].time_point;
        index = i;
      }
    }
    strcpy(cache.data[index].hostname, hostname);
    strcpy(cache.data[index].query, query);
    strncpy(cache.data[index].obj, obj, obj_size);
    cache.data[index].obj_size = obj_size;
    cache.data[index].time_point = timestamp++;
  } else {
    strcpy(cache.data[cache.data_size].hostname, hostname);
    strcpy(cache.data[cache.data_size].query, query);
    strncpy(cache.data[cache.data_size].obj, obj, obj_size);
    cache.data[cache.data_size].obj_size = obj_size;
    cache.data[cache.data_size].time_point = timestamp++;
    cache.data_size++;
  }
  V(&w);
  return 1;
}
