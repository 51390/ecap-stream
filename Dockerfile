# syntax = docker/dockerfile:1.2

FROM ubuntu

ARG ARCH_FLAGS
ARG VALGRIND
ARG VALGRIND_FLAGS
ARG CARGO_HOME

ENV VALGRIND=${VALGRIND}
ENV CARGO_HOME=${CARGO_HOME}

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

# link in gperftools
#ENV LIBS=-ltcmalloc_and_profiler
#ENV CFLAGS=-ltcmalloc_and_profiler
#ENV CXXFLAGS=-ltcmalloc_and_profiler

# libecap
ARG ECAP_BUILD_PATH=/tmp/build-ecap
RUN mkdir $ECAP_BUILD_PATH
WORKDIR $ECAP_BUILD_PATH
RUN wget https://www.e-cap.org/archive/libecap-1.0.0.tar.gz
RUN tar xvpfz libecap-1.0.0.tar.gz
WORKDIR $ECAP_BUILD_PATH/libecap-1.0.0
RUN ./configure ${ARCH_FLAGS} && make && make install
RUN ldconfig

# prism
ARG PRISM_BUILD_PATH=/tmp/build-ecap-stream
ADD . $PRISM_BUILD_PATH
WORKDIR $PRISM_BUILD_PATH
RUN bash bootstrap.sh
RUN ./configure
RUN make
RUN make install
RUN ldconfig
