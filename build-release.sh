#!/bin/sh
DIR=`dirname $(readlink -f $0)`

version=${1:-centos5}

case $version in
  centos5|centos)
    docker build -t buildable-smell-baron -f Dockerfile.centos5 $DIR/ || exit 1
    ;;
  alpine)
    docker build -t buildable-smell-baron -f Dockerfile.alpine $DIR/ || exit 1
    ;;
  *)
    echo "version unsupported, try alpine or centos5"
    exit 1
    ;;
esac

docker run --rm buildable-smell-baron cat /smell-baron/smell-baron  > $DIR/smell-baron || exit 1
docker rmi buildable-smell-baron
