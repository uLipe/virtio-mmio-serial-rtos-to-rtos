/*
 * Copyright (c) 2023, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <openamp/virtio_mmio.h>

static const struct device *ivshmem_dev =
		DEVICE_DT_GET(DT_NODELABEL(ivshmem0));

static const struct device *ipm_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static void ipm_platform_callback(const struct device *ipmdev,
						void *user_data,
			       		uint32_t id,
						volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(id);
	ARG_UNUSED(data);

	extern void virtio_mmio_hvl_cb_run(void);
	virtio_mmio_hvl_cb_run();
}


void virtio_mmio_hvl_ipi(void)
{
	uint16_t peer_dest_id = ivshmem_get_id(ivshmem_dev) - 1;
	ipm_send(ipm_dev, 0, peer_dest_id, NULL, 0);
}

void virtio_mmio_hvl_wait(uint32_t usec)
{
	k_busy_wait(usec);
}

static int init_ivshmem_backend(void)
{
	ipm_register_callback(ipm_dev, ipm_platform_callback, NULL);
	ipm_set_enabled(ipm_dev, 1);
	return 0;
}

SYS_INIT(init_ivshmem_backend, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);