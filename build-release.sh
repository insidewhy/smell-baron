#!/bin/sh
DIR=`dirname $(readlink -f $0)`

version=${1:-centos5}
BNAME="$version-smell-baron-$RANDOM"

case $version in
  centos5|centos)
    docker build -t $BNAME -f Dockerfile.centos5 $DIR/ || exit 1
    ;;
  alpine)
    docker build -t $BNAME -f Dockerfile.alpine $DIR/ || exit 1
    ;;
  debian)
    docker build -t $BNAME -f Dockerfile.debian $DIR/ || exit 1
    ;;
  *)
    echo "version unsupported, try alpine or centos5"
    exit 1
    ;;
esac

docker run --rm $BNAME cat /smell-baron/smell-baron  > $DIR/smell-baron && chmod +x $DIR/smell-baron || exit 1
docker rmi $BNAME
