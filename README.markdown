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
wget https://github.com/ohjames/smell-baron/releases/download/v0.1.0/smell-baron
chmod a+x smell-baron
```

## More stuff

If you want to run multiple processes you can separate them with the argument `---`:
```
ADD smell-baron /bin/smell-baron
ENTRYPOINT ["/bin/smell-baron"]
CMD ["/bin/runit", "---", "/bin/node", "app.js" ]
```
