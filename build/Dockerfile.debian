FROM debian
MAINTAINER Evgeny Kruglov <ekruglov@gmail.com>

RUN apt-get update && apt-get install -y gcc make
ADD . /smell-baron
RUN cd smell-baron && make release
