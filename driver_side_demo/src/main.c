/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>

const struct device *virtio_serial = DEVICE_DT_GET(DT_NODELABEL(virtio_serial0));

/* override the printk output char with our virtio_serial driver: */
int arch_printk_char_out(int c)
{
	if(!device_is_ready(virtio_serial))
		return 0;

	uart_poll_out(virtio_serial, c);
	k_sleep(K_USEC(200));
	return 0;
}

void main(void)
{
	while(1){
		printk("[%d]: Hello World I'm %s sending data over Virtio-MMIO Serial Port\n",
			k_uptime_get_32(),
			CONFIG_BOARD);
	}
}
