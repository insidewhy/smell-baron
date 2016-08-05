#!/bin/sh

version=${1:-centos5}
case $version in
  centos5|centos)
    version=centos5
    ;;
esac

dockerfile=Dockerfile.$version
builddir=dist/$version

if [ ! -f $dockerfile ] ; then
  echo "version unsupported, try alpine or centos5"
  exit 1
fi

docker build -f $dockerfile -t $version-smell-baron . || exit 1
docker run $version-smell-baron true || exit 1
mkdir -p $builddir
docker cp $(docker ps -aq | head -n1):/smell-baron/smell-baron $builddir
