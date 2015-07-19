#!/bin/sh

version=${1:-centos5}

case $version in
  centos5|centos)
    ln -sf Dockerfile.centos5 Dockerfile
    docker build -t centos5-smell-baron . || exit 1
    docker run centos5-smell-baron true || exit 1
    docker cp $(docker ps -aq | head -n1):/smell-baron/smell-baron .
    ;;
  alpine)
    ln -sf Dockerfile.alpine Dockerfile
    docker build -t alpine-smell-baron . || exit 1
    docker run alpine-smell-baron true || exit 1
    docker cp $(docker ps -aq | head -n1):/smell-baron/smell-baron .
    ;;
  *)
    echo "version unsupported, try alpine or centos5"
    ;;
esac
