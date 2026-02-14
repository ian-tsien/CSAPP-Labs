# Solution Process

As a proxy, it is a server to the client and a client to the server. Therefore, we can refer to the code in the `tiny.c` file to organize our logic. However, there are a few points to note:

- **Do not terminate the process once it begins accepting connections.** This means we should not use wrapper functions, which will exit the program directly once any errors happening. As a result, we have to cope with errors by hand, making the program keep running other than exiting.
- **Ignore `SIGPIPE` signals and deal gracefully with `write` operations that return `EPIPE` errors.** This indicates that we need to properly handle cases where the connection is closed prematurely.
- **When the other socket has been prematurely closed, calling `write` will return `-1` with `errno` set to `ECONNRESET`.** Also, we need to handle such case when reading from the client or server.

The reference `proxylab.pdf` describes our implementation of three proxies. We further implement four modes: a sequential mode, two concurrent modes, and a thread-based caching mode.

## A sequential web proxy

We process requests from clients one by one in sequence. This means that only one request is being processed at any given time, therefore the scores for both concurrency and cache are zero.

The scores are listed below:

```txt
*** Basic ***
Starting tiny on 23416
Starting proxy on 21356
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40
```

## Multiple concurrent requests

Using the two methods described in the textbook, we can achieve concurrency through multiprocessing and multithreading.

The results are similar, shown as below:

```txt
*** Concurrency ***
Starting tiny on port 15230
Starting proxy on port 22997
Starting the blocking NOP server on port 7224
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15
```

## Caching web objects

When encountering cache read/write operations, classic read/write problems arise, which can be solved by using locking.

The results are shown below:

```txt
*** Cache ***
Starting tiny on port 24160
Starting proxy on port 2280
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```

None of the code above took into account the format of binary data, so there was a considerable risk, but fortunately it passed all the tests.
