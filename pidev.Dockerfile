FROM arm64v8/alpine

RUN apk add gcompat

RUN apk add --update --no-cache \
    bzip2-dev \
    xz-dev \
    ca-certificates \
    git \
    openssl \
    scons \
    tar \
    w3m \
    unzip \
    make \
    cmake

RUN apk add --no-cache py3-setuptools

# RUN apk add py3-MarkupSafe py3-ecdsa py3-protobuf py3-mnemonic py3-requests py3-flask py3-pytest py3-semver
RUN apk add py3-ecdsa py3-requests py3-flask py3-pytest py3-semver
RUN apk add --update py3-protobuf
RUN apk add --update py3-build

# Install gcc-arm-none-eabi
WORKDIR /root
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi.tar.xz
RUN tar -xvf arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi.tar.xz
RUN cp -r arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi/* /usr/local
RUN rm  arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi.tar.xz
RUN rm -rf arm-gnu-toolchain-12.2.rel1-aarch64-arm-none-eabi

# Install protobuf-compiler
WORKDIR /root
RUN mkdir protoc3
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protoc-3.19.4-linux-aarch_64.zip
RUN unzip protoc-3.19.4-linux-aarch_64 -d protoc3
RUN mv protoc3/bin/* /usr/local/bin
RUN mv protoc3/include/* /usr/local/include
RUN rm -rf protoc3
RUN rm protoc-3.19.4-linux-aarch_64.zip


# Install protobuf/python3 support
WORKDIR /root
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protobuf-python-3.19.4.zip
RUN mkdir protobuf-python
RUN unzip protobuf-python-3.19.4.zip -d protobuf-python

WORKDIR /root/protobuf-python/protobuf-3.19.4/python
RUN python setup.py install
WORKDIR /root
RUN rm protobuf-python-3.19.4.zip

# Install nanopb
WORKDIR /root
RUN git clone --branch v1.0.0 https://github.com/markrypt0/nanopb.git
WORKDIR /root/nanopb/generator/proto
RUN make

RUN rm -rf /root/protobuf-python

# Setup environment
ENV PATH /root/nanopb/generator:$PATH

# Build libopencm3
WORKDIR /root
RUN git clone --branch devdebug-1 https://github.com/markrypt0/libopencm3.git
WORKDIR /root/libopencm3
ENV FP_FLAGS "-mfloat-abi=soft"
RUN make TARGETS='stm32/f2 stm32/f4'

RUN apk add --update --no-cache \
    clang \
    gcc \
    g++ \
    binutils
