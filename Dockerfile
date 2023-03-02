#
# Ubuntu
#
FROM ubuntu:22.04 as base
ARG DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC
RUN apt update && apt -y --no-install-recommends install \
        make \
        cmake \
        git \
        default-jre-headless \
        python3 \
        python3-distutils \
        uuid \
        uuid-dev \
        pkg-config \
        clang \
        libc++1 \
        libc++-dev \
        libc++abi1 \
        libc++abi-dev \
    && rm -rf /var/lib/apt/lists/*
ENV CC clang
ENV CXX clang++

#
# Z3
#
FROM base as z3
RUN git clone --depth=1 --branch z3-4.8.7 https://github.com/Z3Prover/z3.git /artifact/z3/
WORKDIR /artifact/z3/build/
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/usr/
RUN make -j$((`nproc`+1))
RUN make install

#
# Plankton
#
FROM z3 as debug
COPY . /artifact/plankton-debug/source
WORKDIR /artifact/plankton-debug/source/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/artifact -DINSTALL_FOLDER=plankton-debug -DCMAKE_BUILD_TYPE=DEBUG
RUN make -j$((`nproc`+1))
RUN make install

FROM debug as release
COPY . /artifact/plankton/source
WORKDIR /artifact/plankton/source/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/artifact -DINSTALL_FOLDER=plankton -DCMAKE_BUILD_TYPE=RELEASE
RUN make -j$((`nproc`+1))
RUN make install

#
# Start
#
FROM release
WORKDIR /artifact/plankton/
ENTRYPOINT [ "/bin/bash" ]
