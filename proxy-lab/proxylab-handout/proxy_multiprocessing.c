#include "csapp.h"
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
int parse_uri(char *uri, char *hostname, char *query, char *port);
int send_request(int fd, char *hostname, char *query);
int send_back(int fd, rio_t *server_rio);

void sigchld_handler(int sig) {
  while (waitpid(-1, 0, WNOHANG) > 0) {
    ;
  }
  return;
}

int main(int argc, char *argv[]) {
  int listenfd, connfd, rc;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command-line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Capture signal */
  Signal(SIGPIPE, SIG_IGN);
  Signal(SIGCHLD, sigchld_handler);

  listenfd = Open_listenfd(argv[1]);
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
    if (fork() == 0) {
      close(listenfd);
      doit(connfd);
      close(connfd);
      exit(0);
    }
    rc = close(connfd);
    if (rc < 0) {
      fprintf(stderr, "close() error: %s\n", strerror(errno));
    }
  }
}

/*
 * doit - Handle one HTTP request/response transaction
 */
void doit(int fd) {
  int server_fd;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], query[MAXLINE], port[MAXLINE];
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
  if (!send_back(fd, &server_rio)) {
    fprintf(stderr, "send_back() error\n");
    close(server_fd);
    return;
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
  strcpy(buf, "Connetction: close\r\n");
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
int send_back(int fd, rio_t *server_rio) {
  ssize_t n;
  char buf[MAXBUF];

  while ((n = rio_readlineb(server_rio, buf, MAXBUF)) > 0) {
    if (rio_writen(fd, buf, n) <= 0) {
      return 0;
    }
  }
  return 1;
}
