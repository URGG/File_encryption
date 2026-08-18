FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y libssl-dev

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    gdb \
    clang \
    rsync \
    tar \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

LABEL authors="georgeurgiles"