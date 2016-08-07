FROM alpine
MAINTAINER James Pike version: 0.1

RUN apk update && apk add gcc make musl-dev
ADD . /smell-baron
RUN cd smell-baron && make release
