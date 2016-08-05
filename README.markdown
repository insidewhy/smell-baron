# A tiny init for docker containers written in C

If your Dockerfile contains something like this:

```
CMD ["/bin/node", "app.js"]
```

This can lead to [some undesirable consequences](https://blog.phusion.nl/2015/01/20/docker-and-the-pid-1-zombie-reaping-problem/) - in short: your process can get shutdown in a bad way and *zombie processes* will pile up.

To solve this you can use `smell-baron` and change your `Dockerfile` to:

```
ADD smell-baron /bin/smell-baron
ENTRYPOINT ["/bin/smell-baron"]
CMD ["/bin/node", "app.js" ]
```

`smell-baron` is written in `c`, it needs no dependencies on the host machine and consumes almost no resources.

`smell-baron` supplies some extra features:

 * Multiple commands can be run, smell-baron will exit when all the watched processes have exited.
 * Whether a spawned process is watched can be configured.
 * Smell-baron can be told to signal all child processes on termination, this allows it to cleanly deal with processes that spawn processes in different process groups yet fail to clean them up on exit.

## Building

To build a release version:
```
make release
```

To build a debug version:
```
make
```

Or to build a copy against Centos5's `glibc` (so it can run on more machines) first install docker, then run this as a user with access to docker:
```
./build-release.sh
```

It can also be build on alpine to link against `musl` rather than `glibc`:
```
./build-release.sh alpine
```

You can also get a prebuilt binary from github (but don't trust me, build it yourself):

```
wget https://github.com/ohjames/smell-baron/releases/download/v0.3.0/smell-baron
chmod a+x smell-baron
```

## Running more than one process

If you want to run multiple processes you can separate them with the argument `---`:
```
ADD smell-baron /bin/smell-baron
ENTRYPOINT ["/bin/smell-baron"]
CMD ["/bin/runit", "---", "/bin/node", "app.js" ]
```

`smell-baron` will wait for every process to exit before sending a `SIGTERM` to the remaining (reaped) processes. It will exit when all child processes exit.

## Waiting for processes to exit

This behaviour can be altered with the `-f` command-line argument:

```
smell-baron -f sleep 1 --- sleep 2
```

When `-f` is used then only the processes marked with `-f` will be watched, any other processes will be killed with `SIGTERM` after the watched process(es) have exited. In the above example `sleep 2` would be killed after one second. `-f` can be used many times and not specifying it is the same as specifying it before every command.

## Cleaning up

After `smell-baron`'s supervised processes have exited it uses `kill(0, SIGTERM)` to kill remaining processes. This sends a kill signal to every process in the same process group. Some processes create processes in new process groups and then fail to terminate them when they are killed. The `-a` flag can be used to kill all reachable processes (by using `kill(-1, SIGTERM)`). This process can only be used from the init process (a process with pid 1).
