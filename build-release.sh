#!/bin/sh

version=${1:-centos5}

case $version in
  centos5|centos)
    version=centos5
    ;;
  alpine)
    ;;
  *)
    echo "version unsupported, try alpine or centos5"
    exit 1
    ;;
esac

docker build -f Dockerfile.$version -t $version-smell-baron . || exit 1
docker run $version-smell-baron true || exit 1
docker cp $(docker ps -aq | head -n1):/smell-baron/smell-baron .
