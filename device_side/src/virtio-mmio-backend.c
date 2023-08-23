/*
 * Copyright 2023 Linaro.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
/* TODO: get rid of sys_/write/read from zephyr and
 * use metal_io_region instead
 */
#include <zephyr/kernel.h>
#include "virtio-mmio-backend.h"

static METAL_DECLARE_LIST(virtio_mmio_backend_list);

int virtio_mmio_backend_initialize(struct virtio_mmio_backend *dev,
                                uintptr_t config_mem_ptr,
                                uint32_t device_type,
                                virtio_mmio_backend_callback cb,
                                void *user_data)
{
    if(!dev || !config_mem_ptr || !cb)
        return -EINVAL;

    dev->config_mem_base = config_mem_ptr;
    dev->device_type = device_type;
    dev->cb = cb;
    dev->user_data = user_data;
    return 0;
}

int virtio_mmio_backend_register(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    /*Expose in the config shared memory which device we are:*/
    sys_write32(VIRTIO_MMIO_MAGIC_VALUE_STRING,
            dev->config_mem_base + VIRTIO_MMIO_MAGIC_VALUE);

    sys_write32(1,
            dev->config_mem_base + VIRTIO_MMIO_VERSION);

    sys_write32(dev->device_type,
            dev->config_mem_base + VIRTIO_MMIO_DEVICE_ID);

    sys_write32(0,
        dev->config_mem_base + VIRTIO_MMIO_QUEUE_PFN);

    sys_write32(0,
            dev->config_mem_base + VIRTIO_MMIO_INTERRUPT_ACK);

    /* For the HVL Virtio cases we need to provide to the driver
     * side a non-zero len and memory address into the shm register
     */
    uint32_t shm_base = (dev->config_mem_base >> 32);

	sys_write32(shm_base ,dev->config_mem_base + VIRTIO_MMIO_SHM_BASE_HIGH);
	shm_base = dev->config_mem_base & 0xFFFFFFFF;
	sys_write32(shm_base ,dev->config_mem_base + VIRTIO_MMIO_SHM_BASE_LOW);

	sys_write32(0, dev->config_mem_base + VIRTIO_MMIO_SHM_LEN_HIGH);
	sys_write32(0x200, dev->config_mem_base + VIRTIO_MMIO_SHM_LEN_LOW);

    printk("Dump the configuration for the virtio mmio device %p\n", dev);
    printk("VIRTIO_MMIO Device Base address: %p\n",
        (void *)(dev->config_mem_base + VIRTIO_MMIO_MAGIC_VALUE));
    printk("VIRTIO_MMIO_MAGIC_VALUE: 0x%x\n",
        sys_read32(dev->config_mem_base + VIRTIO_MMIO_MAGIC_VALUE));
    printk("VIRTIO_MMIO_VERSION: 0x%x\n",
        sys_read32(dev->config_mem_base + VIRTIO_MMIO_VERSION));
    printk("VIRTIO_MMIO_DEVICE_ID: 0x%x\n",
        sys_read32(dev->config_mem_base + VIRTIO_MMIO_DEVICE_ID));

    metal_list_add_tail(&virtio_mmio_backend_list, &dev->link);

    return 0;
}

int virtio_mmio_backend_get_vring(struct virtio_mmio_backend *dev,
                                struct vring_information *vr)
{

    if(!dev || !dev->config_mem_base || !vr)
        return -EINVAL;

    uintptr_t addr = (uintptr_t)sys_read32(dev->config_mem_base + VIRTIO_MMIO_QUEUE_PFN);
    if(!addr || (addr == 0xAABBAABB)) {
        return -ENOMEM;
    }

    virtio_mmmio_backend_hvl_ack(dev);

    int align = sys_read32(dev->config_mem_base + VIRTIO_MMIO_QUEUE_ALIGN);
    int n = sys_read32(dev->config_mem_base + VIRTIO_MMIO_QUEUE_NUM);
    addr *= align;

    /* setup the vring */
    vr->desc = (void *)addr;
    vr->avail = (void *)((unsigned long)addr +
                n * sizeof(struct vring_desc));
    vr->used = (void *)((unsigned long)addr +
                ((n * sizeof(struct vring_desc) +
                (n + 1) * sizeof(uint16_t) + align - 1) & ~(align - 1)));
    vr->queue_num = n;

    printk("Dump of incoming Vring information from other side:\n");
    printk("vring descriptor: %p:\n", vr->desc);
    printk("vring avail: %p:\n", vr->avail);
    printk("vring used: %p:\n", vr->used);
    printk("vring entries: %d:\n", vr->queue_num);

    if(!vr->desc || !vr->avail || !vr->used)
        return -ENODEV;

    return 0;
}

int virtio_mmio_backend_raise_interrupt(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    sys_write32(VIRTIO_MMIO_INT_VRING,
            dev->config_mem_base + VIRTIO_MMIO_INTERRUPT_STATUS);

    return 0;
}

uint32_t virtio_mmio_backend_get_status(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    return (sys_read32(dev->config_mem_base + VIRTIO_MMIO_STATUS));
}

int virtio_mmio_backend_get_active_queue(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    return (int)(sys_read32(dev->config_mem_base + VIRTIO_MMIO_QUEUE_NOTIFY));
}

int virtio_mmio_backend_get_queue_id(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    return (int)(sys_read32(dev->config_mem_base + VIRTIO_MMIO_DRIVER_FEATURES_SEL));
}

int virtio_mmmio_backend_hvl_ack(struct virtio_mmio_backend *dev)
{
    if(!dev)
        return -EINVAL;

    sys_write32(0xAABBAABB,
            dev->config_mem_base + VIRTIO_MMIO_QUEUE_PFN);

    return 0;
}

int virtio_mmio_backend_isr(void)
{
    struct metal_list *node;
    struct virtio_mmio_backend *dev;

    metal_list_for_each(&virtio_mmio_backend_list, node) {
		dev = metal_container_of(node, struct virtio_mmio_backend, link);

        uint32_t ack = sys_read32(dev->config_mem_base +
                                VIRTIO_MMIO_INTERRUPT_ACK);

        if(ack & VIRTIO_MMIO_INT_VRING) {
            /* just an int ack, ignore it */
            sys_write32(0,
                dev->config_mem_base + VIRTIO_MMIO_INTERRUPT_ACK);
            return 0;
        }

        /* read status and defer callback if needed */
        uint32_t status = sys_read32(dev->config_mem_base +
                                VIRTIO_MMIO_STATUS);

        if(dev->cb)
            dev->cb(dev, status, dev->user_data);
	}

    return 0;
}
