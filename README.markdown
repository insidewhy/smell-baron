# A tiny init for docker containers written in C

Hey maybe your Docker file contains something like this:

```
CMD ["/bin/node", "app.js"]
```

Oh no [this is no good!](https://blog.phusion.nl/2015/01/20/docker-and-the-pid-1-zombie-reaping-problem/) - TLDR: Your process can get shutdown in a bad way and *zombie processes* will eat your container!

Instead just use `smell-baron`:

1. Download it (this binary will work on any Linux version from Centos-5/Redhat-5 and up):

    ```
    wget https://github.com/ohjames/smell-baron/releases/download/v0.1.0/smell-baron
    chmod a+x lovely_touching
    ```

2. Change your `Dockerfile` to this:

    ```
    ADD smell-baron /bin/smell-baron
    CMD ["/bin/smell-baron", "/bin/node", "app.js" ]
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

Or to build a copy against Centos5's glibc (so it can run on more machines) first install docker, then run this as a user with access to docker:

```
./build-release.sh
```

## More stuff

If you want to run multiple processes you can separate them with the argument `---`:
```
ADD smell-baron /bin/smell-baron
CMD ["/bin/smell-baron", "/bin/runit", "---", "/bin/node", "app.js" ]
```
