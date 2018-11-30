FROM ubuntu:18.04

RUN apt-get update
RUN apt-get install -y meson python3 ninja-build
RUN apt-get install -y libuv1-dev libiniparser-dev libjansson-dev libeina-dev

RUN mkdir -p /opt/zigbridge
WORKDIR /opt/zigbridge

COPY . .

RUN mkdir -p /etc/zigbdrige && cp config/config_sample.ini /etc/zigbdrige/config.ini

RUN meson builddir
RUN ninja -C builddir