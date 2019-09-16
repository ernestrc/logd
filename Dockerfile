FROM centos:7

RUN yum install -y epel-release && yum update -y
RUN yum install -y git make gcc libtool zlib-devel openssl-devel patch vim-common

WORKDIR /opt/logd
VOLUME /opt/logd/bin

COPY . /opt/logd

RUN ./configure --enable-build-luajit --enable-build-openssl --enable-build-libuv && make && make install

CMD /opt/logd/bin/logd
