# use ubuntu bionic, as we need openssl 1.0.2
FROM ubuntu:18.04 as build

RUN apt-get update -y
RUN apt-get install -y apt-transport-https ca-certificates git make gcc libtool zlib1g-dev patch vim-common libluajit-5.1-dev pkg-config autoconf libssl-dev curl

WORKDIR /opt/logd
COPY . /opt/logd
RUN ./configure --enable-build-libuv CFLAGS='-I/opt/logd/deps/include -I/usr/include/luajit-2.1 -O3 -DOPENSSL_NO_STDIO -DOPENSSL_NO_SM2' && make && make install
RUN mkdir -p /opt/logd/luasrc &&\
	cd /opt/logd/luasrc &&\
	curl -L https://github.com/luvit/lit/raw/master/get-lit.sh | sh && \
	mv ./lit /opt/logd/bin/lit

# clean image for running it, only with lib dependencies
FROM ubuntu:18.04
WORKDIR /opt/logd
RUN apt-get update -y
RUN apt-get install -y zlib1g-dev libluajit-5.1-dev libssl-dev

COPY --from=build /opt/logd/bin /opt/logd/bin
COPY --from=build /opt/logd/luasrc /opt/logd/src
CMD /opt/logd/bin/logd
