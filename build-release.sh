#!/bin/sh

docker build -t centos5-smell-baron . || exit 1
docker run centos5-smell-baron true || exit 1
docker cp $(docker ps -aq | head -n1):/smell-baron/smell-baron .
