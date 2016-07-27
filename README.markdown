# A tiny init for docker containers written in C

Hey maybe your Docker file contains something like this:

```
CMD ["/bin/node", "app.js"]
```

Oh no [this is no good!](https://blog.phusion.nl/2015/01/20/docker-and-the-pid-1-zombie-reaping-problem/) - TLDR: Your process can get shutdown in a bad way and *zombie processes* will eat your container!

Instead just use `smell-baron` and change your `Dockerfile` to:

```
ADD smell-baron /bin/smell-baron
ENTRYPOINT ["/bin/smell-baron"]
CMD ["/bin/node", "app.js" ]
```

Now you don't have to worry anymore!

`smell-baron` is written in `c` so you don't need anything installed in the host machine to use the binary and it consumes almost no resources.

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

After `smell-baron`'s supervised processes have exited it uses `kill(0, SIGTERM)` to kill remaining processes. This sends a kill signal to every process in the same process group. Some processes create processes in new process groups and then fail to terminate them when they are killed. The `-a` flag can be used to kill all reachable processes (by using `kill(1, SIGTERM)`). It is not advisable to use this outside of a container.
