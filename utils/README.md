# Utils

## Docker images
Several Docker images are provided to ease compilation to other systems and Linux distributions.

1. First build docker image:
```sh
$ docker build -t logd-build:centos . -f utils/Dockerfile.centos7
```

2. Then compile:
```sh
$ docker run --rm -v /tmp/logd_centos:/opt/logd/bin logd-build:centos
```

3. Executable can be found in the host's mounted volume


## Musl libc
For maximum application deployability, you can use the provided [Dockerfile.static](Dockerfile.static) which compiles and statically links a logd executable against musl libc.
