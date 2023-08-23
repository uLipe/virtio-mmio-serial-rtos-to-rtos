# Copyright (c) 2023 Linaro
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

#list(APPEND QEMU_EXTRA_FLAGS -S)
#list(APPEND QEMU_EXTRA_FLAGS -s)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_mmmio_serial)

target_sources(app PRIVATE src/virtio-mmio-backend-serial.c
        src/virtio-mmio-backend.c)
