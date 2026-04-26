FROM ubuntu:22.04

# Set non-interactive mode for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install required dependencies
RUN apt update && apt install -y \
    build-essential \
    bison \
    flex \
    libncurses-dev \
    wget \
    tar \
    gzip \
    xz-utils \
    qemu-system-x86 \
    git \
    bc \
    cpio \
    rsync \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /buildroot-work

# Clone buildroot
RUN git clone https://git.busybox.net/buildroot

WORKDIR /buildroot-work/buildroot

# Create minimal configuration
RUN echo 'BR2_x86=y\n\
BR2_TOOLCHAIN_BUILDROOT=y\n\
BR2_SYSTEM_BIN_SH_BUSYBOX=y\n\
BR2_PACKAGE_BUSYBOX=y\n\
BR2_PACKAGE_BUSYBOX_CONFIG="package/busybox/busybox-minimal.config"\n\
BR2_TARGET_ROOTFS_CPIO=y\n\
BR2_TARGET_ROOTFS_TAR=y' > configs/minimal_linux_defconfig

CMD ["/bin/bash"]
