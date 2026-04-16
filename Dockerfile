FROM --platform=linux/amd64 ubuntu:24.04 AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

# Build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    texinfo \
    wget \
    xorriso \
    grub-pc-bin \
    grub-common \
    grub2-common \
    mtools \
    qemu-system-x86 \
    nasm \
    && rm -rf /var/lib/apt/lists/*

ENV PREFIX="/opt/cross"
ENV TARGET=i686-elf
ENV PATH="$PREFIX/bin:$PATH"

# Build binutils
WORKDIR /tmp/src
RUN wget -q https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz \
    && tar xf binutils-2.42.tar.xz \
    && mkdir binutils-build && cd binutils-build \
    && ../binutils-2.42/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --with-sysroot \
        --disable-nls \
        --disable-werror \
    && make -j$(nproc) \
    && make install \
    && cd /tmp && rm -rf src

# Build GCC (cross-compiler)
WORKDIR /tmp/src
RUN wget -q https://ftp.gnu.org/gnu/gcc/gcc-14.1.0/gcc-14.1.0.tar.xz \
    && tar xf gcc-14.1.0.tar.xz \
    && mkdir gcc-build && cd gcc-build \
    && ../gcc-14.1.0/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-nls \
        --enable-languages=c \
        --without-headers \
    && make -j$(nproc) all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc \
    && cd /tmp && rm -rf src

# Build Newlib
WORKDIR /tmp/src
RUN wget -q ftp://sourceware.org/pub/newlib/newlib-4.4.0.20231231.tar.gz \
    && tar xf newlib-4.4.0.20231231.tar.gz \
    && mkdir newlib-build && cd newlib-build \
    && ../newlib-4.4.0.20231231/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-libgloss \
    && make -j$(nproc) \
    && make install \
    && cd /tmp && rm -rf src

# --- Final stage (smaller image) ---
FROM --platform=linux/amd64 ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    xorriso \
    grub-pc-bin \
    grub-common \
    grub2-common \
    mtools \
    qemu-system-x86 \
    nasm \
    && rm -rf /var/lib/apt/lists/*

ENV PREFIX="/opt/cross"
ENV TARGET=i686-elf
ENV PATH="$PREFIX/bin:$PATH"

# Copy the cross-toolchain + newlib from the build stage
COPY --from=toolchain $PREFIX $PREFIX

WORKDIR /kernel
