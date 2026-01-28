# Solution Process

We need to implement seven functions listed below and verify them by 16 trace files:

1. `eval`: Main routine that parses and interprets the command line.
2. `builtin_cmd`: Recognizes and interprets the built-in commands: `quit`, `fg`, `bg`, and `jobs`.
3. `do_bgfg`: Implements the `bg` and `fg` built-in commands.
4. `waitfg`: Waits for a foreground job to complete.
5. `sigchld_handler`: Catches `SIGCHILD` signals.
6. `sigint_handler`: Catches `SIGINT` signals.
7. `sigtstp_handler`: Catches `SIGTSTP` signals.

The three signal processing functions are logically independent, so we will implement them first.

First, function `sigtstp_handler` sends a `SIGTSTP` signal to a foreground job. We need to block the `SIGCHLD` signal when reading the global array `jobs` to ensure the accuracy of the read data. Also, for the signal handler function, be careful to restore the value of `errno` to prevent disrupting the main workflow.

The implementation code is as follows:

```c
void sigtstp_handler(int sig) {
  int olderrno = errno;
  pid_t pid;
  sigset_t mask, prev_mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);

  sigprocmask(SIG_BLOCK, &mask, &prev_mask);
  pid = fgpid(jobs); /* Get pid of foreground job */
  sigprocmask(SIG_SETMASK, &prev_mask, NULL);
  if (pid) {
    kill(-pid, SIGTSTP);
  }

  errno = olderrno;
}
```

Note that we only need to disable the `SIGCHLD` signal, because only the `SIGCHLD` signal handler modifies the contents of `jobs`. Function `sigint_handler` also only reads content of `jobs`, so it can be executed concurrently. This is somewhat similar to the principle of read-write locks.

The code for function `sigint_handler` is similar:

```c
void sigint_handler(int sig) {
  int olderrno = errno;
  pid_t pid;
  sigset_t mask, prev_mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);

  sigprocmask(SIG_BLOCK, &mask, &prev_mask);
  pid = fgpid(jobs); /* Get pid of foreground job */
  sigprocmask(SIG_SETMASK, &prev_mask, NULL);
  if (pid) {
    kill(-pid, SIGINT);
  }

  errno = olderrno;
}
```

The situation is different for function `sigchld_handler`. Because we need to modify or delete elements in `jobs`, we need to block not only the `SIGCHLD` signal, but also the other two signals. Otherwise, it will cause serious problems.

For example, consider a stopped job. If function `sigint_handler` executes before function `sigchld_handler` modifies the corresponding job's state to `ST`, it will send a `SIGINT` signal to a job that is no longer `FG`. This will lead to logical inconsistencies.

We also need to notice that we shouldn't use `printf` in the `sigchld_handler`, because the function `printf` is a non-reentrant function. Instead, we use functions `snprintf` and `write` to output the string to `stdout`.

The code for function `sigchld_handler` is as follows:

```c
void sigchld_handler(int sig) {
  static char buf[128]; /* Holds buffer for write */
  int len;              /* Length of output */
  int olderrno = errno;
  pid_t pid;
  int status;
  struct job_t *job;
  sigset_t mask, prev_mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTSTP);
  sigaddset(&mask, SIGCHLD);

  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    if (WIFEXITED(status)) { /* Exit normally */
      deletejob(jobs, pid);
    } else if (WIFSIGNALED(status)) { /* Exit by signal */
      len =
          snprintf(buf, sizeof(buf), "Job [%d] (%d) terminated by signal %d\n",
                   pid2jid(pid), pid, WTERMSIG(status));
      write(STDOUT_FILENO, buf, len);
      deletejob(jobs, pid);
    } else if (WIFSTOPPED(status)) { /* Stopped by signal */
      len = snprintf(buf, sizeof(buf), "Job [%d] (%d) stopped by signal %d\n",
                     pid2jid(pid), pid, WSTOPSIG(status));
      write(STDOUT_FILENO, buf, len);
      job = getjobpid(jobs, pid);
      job->state = ST;
    }
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);
  }

  errno = olderrno;
}
```

Then, we implement the function `waitfg` followed by `do_bgfg`, since `waitfg` is relied by `do_bgfg`.

The function `waitfg` is simple and easy to understand. It merely unblocks all signals and suspends the shell process. When the handler deleted or modified the foreground job, which means the foreground jobs has been terminated of stopped, the shell process restarts to run.

The code for function `waitfg` is as follows:

```c
void waitfg(pid_t pid) {
  sigset_t mask;

  sigemptyset(&mask);

  while (fgpid(jobs) != 0) { /* The foreground job terminated or stopped? */
    sigsuspend(&mask);       /* Unblock SIGCHLD */
  }
}
```

The function `do_bgfg` is more complicated, because it not only reads data, but also modifies data. To minimize the range of blocked signals, we only block `SIGCHLD` when read data and block three signals when modify data. `do_bgfg` serves in the main routine, so it can use functions such as `printf`.

The code is as follows:

```c
void do_bgfg(char **argv) {
  int id;            /* Holds jid or pid */
  int state;         /* The command fg or bg? */
  char prefix;       /* The first character of the second argument */
  struct job_t *job; /* Holds job processed */
  sigset_t mask_one, mask_all, prev_mask;

  sigemptyset(&mask_one);
  sigaddset(&mask_one, SIGCHLD);
  sigemptyset(&mask_all);
  sigaddset(&mask_all, SIGINT);
  sigaddset(&mask_all, SIGTSTP);
  sigaddset(&mask_all, SIGCHLD);

  /* Parse and interpret arguments */
  state = (!strcmp(argv[0], "fg")) ? FG : BG;
  if (argv[1] == NULL) { /* No jid or pid */
    printf("%s command requires PID or %%jobid argument\n", argv[0]);
    return;
  }
  prefix = argv[1][0];
  sigprocmask(SIG_BLOCK, &mask_one, &prev_mask); /* Block SIGCHLD */
  if (prefix == '%') {                           /* It's jid */
    id = atoi(&argv[1][1]);
    job = getjobjid(jobs, id);
    if (job == NULL) { /* Invalid jid */
      printf("%s: No such job\n", argv[1]);
      sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* Unblock SIGCHLD */
      return;
    }
  } else if (isdigit(prefix)) { /* It's pid */
    id = atoi(argv[1]);
    job = getjobpid(jobs, id);
    if (job == NULL) { /* Invalid pid */
      printf("(%d): No such process\n", id);
      sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* Unblock SIGCHLD */
      return;
    }
  } else { /* Wrong input */
    printf("%s: argument must be a PID or %%jobid\n", argv[0]);
    sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* Unblock SIGCHLD */
    return;
  }

  /* Restart the child job */
  sigprocmask(SIG_BLOCK, &mask_all, NULL); /* Block SIGCHLD, SIGTSTP, SIGINT */
  job->state = state;
  sigprocmask(SIG_SETMASK, &mask_one, NULL); /* Unblock SIGTSTP, SIGINT */
  kill(-(job->pid), SIGCONT);
  if (state == BG) {
    // Since the `job->cmdline` ends with `\n`, we don't need to append it to
    // the format string.
    printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
  } else {
    waitfg(job->pid);
  }
  sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* Unblock SIGCHLD */
}
```

The function `builtin_cmd` is quite simple. Notice to block `SIGCHLD` signal when read data from `jobs`:

```c
int builtin_cmd(char **argv) {
  char *name = argv[0];
  sigset_t mask, prev_mask;

  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);

  if (!strcmp(name, "quit")) { /* Input quit */
    exit(0);
  } else if (!strcmp(name, "jobs")) {          /* Input jobs */
    sigprocmask(SIG_BLOCK, &mask, &prev_mask); /* Block SIGCHLD */
    listjobs(jobs);
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);            /* Unblock SIGCHLD */
  } else if (!strcmp(name, "bg") || !strcmp(name, "fg")) { /* Input bg or fg */
    do_bgfg(argv);
  } else { /* Not a builtin command */
    return 0;
  }
  return 1;
}
```

Finally, we come to implement the function `eval`, a function evaluating the command line that the user has just typed in. Similar to `do_bgfg`, we divide the function by two block ranges, one for reading data and the other for modifying data.
We have to restore the signal block state for the child job, and put the child in a new process group whose group ID is identical to the child's PID. This makes sure that when we terminate or stop a job, it will not apply the corresponding action to the shell process.

The code is shown below:

```c
void eval(char *cmdline) {
  static char *argv[MAXARGS]; /* Holds arguments of command line */
  int bg;                     /* Background job? */
  pid_t pid;
  sigset_t mask_one, mask_all, prev_mask;

  sigemptyset(&mask_one);
  sigaddset(&mask_one, SIGCHLD);
  sigemptyset(&mask_all);
  sigaddset(&mask_all, SIGINT);
  sigaddset(&mask_all, SIGTSTP);
  sigaddset(&mask_all, SIGCHLD);

  /* Parse command line */
  bg = parseline(cmdline, argv);
  if (argv[0] == NULL) /* Ignore blank line */
    return;

  /* Process as built-in command */
  if (builtin_cmd(argv))
    return;

  /* Process the executable file */
  sigprocmask(SIG_BLOCK, &mask_one, &prev_mask); /* Block SIGCHLD */
  if ((pid = fork()) == 0) {                     /* Child process */
    sigprocmask(SIG_SETMASK, &prev_mask,
                NULL); /* Unblock SIGCHLD for the child process */
    setpgid(0, 0);     /* Add the child process into a new group */
    if (execve(argv[0], argv, environ) < 0) { /* Invalid executable file */
      printf("%s: Command not found\n", argv[0]);
      exit(0);
    }
  }
  if (bg) { /* Run background */
    sigprocmask(SIG_BLOCK, &mask_all,
                NULL); /* Block SIGCHLD, SIGTSTP, SIGINT */
    addjob(jobs, pid, BG, cmdline);
    sigprocmask(SIG_SETMASK, &mask_one, NULL); /* Unblock SIGTSTP, SIGINT */
    printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
  } else { /* Run foreground */
    sigprocmask(SIG_BLOCK, &mask_all,
                NULL); /* Block SIGCHLD, SIGTSTP, SIGINT */
    addjob(jobs, pid, FG, cmdline);
    sigprocmask(SIG_SETMASK, &mask_one, NULL); /* Unblock SIGTSTP, SIGINT */
    waitfg(pid);
  }
  sigprocmask(SIG_SETMASK, &prev_mask, NULL); /* Unblock SIGCHLD */
}
```

We compile this file by `make` command. Then, we compare the output with the content of `tshref.out` to check the correctness.

The test result of `tsh.c` has been proved right, and we have completed this lab.
