# syntax = docker/dockerfile:1.2

FROM ubuntu

# tz
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# package dependencies
RUN apt update -y --fix-missing; \
    apt install -y \
    curl wget gcc g++ automake autoconf libtool openssl \
    libssl-dev libcppunit-dev make pkg-config vim gdb zstd \
    zlib1g-dev valgrind rsyslog build-essential \
    google-perftools libgoogle-perftools-dev smem

# arch flags
ARG ARCH_SCRIPT=/tmp/arch.sh
RUN echo 'test $(uname -i) = "aarch64" && echo -n "--build=aarch64-unknown-linux-gnu"' > $ARCH_SCRIPT

# libecap
ARG ECAP_BUILD_PATH=/tmp/build-ecap
RUN mkdir $ECAP_BUILD_PATH
WORKDIR $ECAP_BUILD_PATH
RUN wget https://www.e-cap.org/archive/libecap-1.0.0.tar.gz
RUN tar xvpfz libecap-1.0.0.tar.gz
WORKDIR $ECAP_BUILD_PATH/libecap-1.0.0
RUN ./configure `bash $ARCH_SCRIPT`  && make && make install
RUN ldconfig

# ecap-stream
ARG ECAP_STREAM_BUILD_PATH=/tmp/build-ecap-stream
ADD . $ECAP_STREAM_BUILD_PATH
WORKDIR $ECAP_STREAM_BUILD_PATH
RUN bash bootstrap.sh
RUN ./configure
RUN make
RUN make install
RUN ldconfig
