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
docker create $version-smell-baron || exit 1
mkdir -p $builddir
containerid=$(docker ps -aq | head -n1)
docker cp $containerid:/smell-baron/smell-baron $builddir
docker rm $containerid
