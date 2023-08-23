/*
 * Copyright 2023 Linaro.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __VIRTIO_MMIO_BACKEND_H
#define __VIRTIO_MMIO_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <openamp/virtqueue.h>
#include <openamp/virtio_ring.h>
#include <openamp/virtio.h>
#include <openamp/virtio_mmio.h>
#include <openamp/virtio_mmio_hvl.h>
#include <metal/device.h>

struct vring_information {
	volatile struct vring_desc *desc;
	volatile struct vring_avail *avail;
	volatile struct vring_used *used;
    int queue_num;
};

struct virtio_mmio_backend;

typedef void (*virtio_mmio_backend_callback) (struct virtio_mmio_backend *dev,
                                    uint32_t status,
                                    void *argument);

struct virtio_mmio_backend {
    struct metal_list link;
    uintptr_t config_mem_base;
    uint32_t device_type;
    virtio_mmio_backend_callback cb;
    void *user_data;
};

int virtio_mmio_backend_initialize(struct virtio_mmio_backend *dev,
                            uintptr_t config_mem_ptr,
                            uint32_t device_type,
                            virtio_mmio_backend_callback cb,
                            void *user_data);

int virtio_mmio_backend_register(struct virtio_mmio_backend *dev);

int virtio_mmio_backend_get_vring(struct virtio_mmio_backend *dev,
                                struct vring_information *vr);

int virtio_mmio_backend_get_active_queue(struct virtio_mmio_backend *dev);

int virtio_mmio_backend_get_queue_id(struct virtio_mmio_backend *dev);

int virtio_mmio_backend_raise_interrupt(struct virtio_mmio_backend *dev);

uint32_t virtio_mmio_backend_get_status(struct virtio_mmio_backend *dev);

int virtio_mmmio_backend_hvl_ack(struct virtio_mmio_backend *dev);

int virtio_mmio_backend_isr(void);


#endif