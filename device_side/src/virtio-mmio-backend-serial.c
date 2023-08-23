/*
 * Copyright 2023 Linaro.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/drivers/uart.h>
#include <openamp/virtio_serial.h>
#include "virtio-mmio-backend.h"

const struct device *ivshmem_dev = DEVICE_DT_GET(DT_NODELABEL(ivshmem0));
const struct device *ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm_ivshmem0));
const struct device *serial_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

enum virtio_serial_backend_state {
	VIRTIO_SERIAL_DEV_READY = 0,
	VIRTIO_SERIAL_DEV_CFG_ACK,
	VIRTIO_SERIAL_DEV_CFG_DRIVER,
	VIRTIO_SERIAL_DEV_SET_FEATURES,
	VIRTIO_SERIAL_DEV_UNKNOWN,
};

static struct virtio_mmio_backend_serial {
	struct virtio_mmio_backend virtio_mmio_backend_dev;
	struct vring_information vrings[2];
	int vq_count;
	enum virtio_serial_backend_state state;
} serial_backend_dev = {.vq_count = 2, .state = VIRTIO_SERIAL_DEV_UNKNOWN};


static void ipm_platform_callback(const struct device *ipmdev, void *user_data,
			       uint32_t id, volatile void *data)
{
	ARG_UNUSED(ipmdev);
	ARG_UNUSED(user_data);
	virtio_mmio_backend_isr();
}

void virtio_mmio_backend_handler(struct virtio_mmio_backend *dev,
							uint32_t status,
							void *argument)
{
	if (status & VIRTIO_CONFIG_STATUS_DRIVER_OK && (serial_backend_dev.state != VIRTIO_SERIAL_DEV_READY)) {
		printk("Config Status DRIVER_OK \n");
		serial_backend_dev.state = VIRTIO_SERIAL_DEV_READY;
		return;

	} else if (serial_backend_dev.state == VIRTIO_SERIAL_DEV_UNKNOWN ) {
		/* virtqueue setup phase */
		int idx = virtio_mmio_backend_get_queue_id(dev);
		virtio_mmio_backend_get_vring(dev, &serial_backend_dev.vrings[idx]);

	} else if (serial_backend_dev.state == VIRTIO_SERIAL_DEV_READY) {
		int queue_id = virtio_mmio_backend_get_active_queue(dev);

		uint16_t index = serial_backend_dev.vrings[queue_id].used->idx %
					serial_backend_dev.vrings[queue_id].queue_num;
		uint16_t desc_index = serial_backend_dev.vrings[queue_id].avail->ring[index];

		if (queue_id == 0 &&
				(void *)serial_backend_dev.vrings[queue_id].desc[desc_index].addr != NULL) {

			/* is a poll in operation */
			if(!uart_poll_in(serial_dev,
						(uint8_t *)&serial_backend_dev.vrings[queue_id].desc[desc_index].addr)) {
				/* received data, forward this data to the driver */
				serial_backend_dev.vrings[queue_id].used->ring[index].len = 1;
			} else {
				serial_backend_dev.vrings[queue_id].used->ring[index].len = 0;
			}

		} else if (queue_id == 1 &&
					(void *)serial_backend_dev.vrings[queue_id].desc[desc_index].addr != NULL) {
			/* is a poll out operation */
			uint8_t c = *(uint8_t*)(serial_backend_dev.vrings[queue_id].desc[desc_index].addr);

			uart_poll_out(serial_dev, c);
			serial_backend_dev.vrings[queue_id].used->ring[index].len = 1;
		} else {
			/* descriptor is invalid, skip it:*/
			printk("Invalid descriptor! \n");
			return;
		}

		serial_backend_dev.vrings[queue_id].used->ring[index].id =
								serial_backend_dev.vrings[queue_id].avail->ring[index];
		serial_backend_dev.vrings[queue_id].used->idx++;

		virtio_mmio_backend_raise_interrupt(dev);
		ipm_send(ipm_dev, 0, ivshmem_get_id(ivshmem_dev) + 1, NULL, 0);
	}
}

int main(void)
{
	uintptr_t ivhsmem_cfg_area;

	int ivshmem_area_size = ivshmem_get_mem(ivshmem_dev, &ivhsmem_cfg_area);

	/* device side initializes IVSHMEM before usage*/
	memset((void *)ivhsmem_cfg_area, 0, ivshmem_area_size);

	ipm_register_callback(ipm_dev, ipm_platform_callback, NULL);
	ipm_set_enabled(ipm_dev, 1);

	virtio_mmio_backend_initialize(&serial_backend_dev.virtio_mmio_backend_dev,
						ivhsmem_cfg_area,
						VIRTIO_ID_CONSOLE,
						virtio_mmio_backend_handler,
						NULL);

	virtio_mmio_backend_register(&serial_backend_dev.virtio_mmio_backend_dev);

	return 0;
}
