Virtio-mmio Serial over IVSHMEM Zephyr Demo
###########################################

Prerequisites
*************

* QEMU needs to available.

* ivshmem-server needs to be available and running. The server is available in
Zephyr SDK or pre-built in some distributions. Otherwise, it is available in
QEMU source tree.

* ivshmem-client needs to be available as it is employed in this sample as an
external application. The same conditions of ivshmem-server apply to the
ivshmem-server, as it is also available via QEMU.

* this sample assumes you have Zephyr SDK 0.16 or above.

Preparing IVSHMEM server
************************
#. The ivshmem-server utility for QEMU can be found into the Zephyr SDK
   directory, in:
   ``/path/to/your/zephyr-sdk/zephyr-<version>/sysroots/x86_64-pokysdk-linux/usr/xilinx/bin/``

#. You may also find ivshmem-client utility, it can be useful to check if everything works
   as expected.

#. Run ivshmem-server. For the ivshmem-server, both number of vectors and
   shared memory size are decided at run-time (when the server is executed).
   For Zephyr, the number of vectors and shared memory size of ivshmem are
   decided at compile-time and run-time, respectively. For Arm64 we use
   vectors == 2 for the project configuration in this sample. Here is an example:

   .. code-block:: console

      # n = number of vectors
      $ sudo ivshmem-server -n 2
      $ *** Example code, do not use in production ***

#. Appropriately set ownership of ``/dev/shm/ivshmem`` and
   ``/tmp/ivshmem_socket`` for your deployment scenario. For instance:

   .. code-block:: console

      $ sudo chgrp $USER /dev/shm/ivshmem
      $ sudo chmod 060 /dev/shm/ivshmem
      $ sudo chgrp $USER /tmp/ivshmem_socket
      $ sudo chmod 060 /tmp/ivshmem_socket

Setting up the environment
**************************
This repository works with Zephyr west metatool, the quickiest way to setup
everything is using west in your host computer, first create a workspace
folder where west should be initialized:

    .. code-block:: console

      $ mkdir virtio_mmio_ws
      $ cd virtio_mmio_ws

Now initialize west and update it:

    .. code-block:: console

      $ west init -m git@github.com:uLipe/virtio-mmio-serial-rtos-to-rtos
      $ west update

After the completion of the commands above you might end-up with the application
folders and the zephyr kernel pointing a patched branch that also brings up the
openamp virtio-exp branch that contains the virtio-mmio implementation for
driver side (the device side also consumes some of those files)

Building and Running
********************

After getting QEMU ready to go, and setting up the repository, first create two output folders, inside of
the workspace folder created earlier, we will create two zephyr application, so open two terminals
and create them, these folders will receive the output of Zephyr west commands:

   .. code-block:: console

      $ mkdir instance_1

On another terminal window do:

   .. code-block:: console

      $ mkdir instance_2

Before building, open the file in ``modules/lib/open-amp/cmake/options.cmake``, navigate
until you find this line:

``option (WITH_HVL_VIRTIO "Build with hypervisor-less virtio (front end) enabled" OFF)``

Modify it to ON, this will enable the virtio-mmio in hypervisor-less mode, which is required
for the driver side:

``option (WITH_HVL_VIRTIO "Build with hypervisor-less virtio (front end) enabled" ON)``

Then build the sample as follows, don't forget, two builds are necessary
to test this sample, so append the option ``-d instance_1`` and
on the other terminal window do the same, that is it ``-d instance_2``

.. code-block:: console

   $ west build -palways -bqemu_cortex_a53 -d instance_1 virtio-mmio-serial-rtos-to-rtos/device_side

On the other terminal window build the device side:

.. code-block:: console

   $ west build -palways -bqemu_cortex_a53 -d instance_2 virtio-mmio-serial-rtos-to-rtos/driver_side_demo

To run the instances, first notice, there is a limitation imposed by IVSHMEM driver to discover where
are the peer-ids, so the application in both device and driver side hardcodes their peer-ids. With this
observation you MUST start first the device side followed by the driver side, without any other qemu instances
that uses IVSHMEM in the middle, any other order will FAIL and the driver and device side cross interrupt
will never reach each other.

So first go to the terminal where you built the instance for the device side, in this case ``instance_1``
and use the west command to run it:

.. code-block:: console

   $ west build -t run -d instance_1

Now go to the ``instance_2`` or where you built the driver instance and start it in the
same way:

.. code-block:: console

   $ west build -t run -d instance_2


Expected output
***************

After running the instances, check the terminal in the ``instance_1``, the output below
will represent the negotiation phase, that is it the driver is setting up the virtqueues
and it also exchange the features and VRINGS addresses to the device:

.. code-block:: console

    *** Booting Zephyr OS build v1.8.99-67522-gc40fbdfc4fcd ***
    Dump the configuration for the virtio mmio device 0x4019ae60
    VIRTIO_MMIO Device Base address: 0xafa00000
    VIRTIO_MMIO_MAGIC_VALUE: 0x74726976
    VIRTIO_MMIO_VERSION: 0x1
    VIRTIO_MMIO_DEVICE_ID: 0x3
    Dump of incoming Vring information from other side:
    vring descriptor: 0xafb01000:
    vring avail: 0xafb01040:
    vring used: 0xafb02000:
    vring entries: 4:
    Dump of incoming Vring information from other side:
    vring descriptor: 0xafb03000:
    vring avail: 0xafb03040:
    vring used: 0xafb04000:
    vring entries: 4:
    Config Status DRIVER_OK

The last message from configuration phase is the ``Config Status DRIVER_OK``,
when the driver succesfully negotiates with the device this message is shown
repressenting the virtio-mmio device was able to read the ``DRIVER_OK`` from
status register.

After some instants, the driver side will start to call the ``printk`` sequentially
this will redirects the characters to be placed into the virtio-serial driver
underneath of the Zephyr uart driver, the data flow will trigger the virtio-mmio
device side and this instance will start to show what the driver instance is
writing and then, finally, the device will intercepts this data and routes to its
actual serial driver so in the device console you might able to see what driver
writes:

.. code-block:: console

    Config Status DRIVER_OK
    [8010]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [9650]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [11290]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [12950]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [14610]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [16270]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [17930]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [19590]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [21250]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [22910]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [24570]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [26230]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [27890]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [29550]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [31210]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [32870]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [34530]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [36190]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [37850]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [39510]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port
    [41170]: Hello World I'm qemu_cortex_a53 sending data over Virtio-MMIO Serial Port

Discussions about this implementation
#####################################

To make this implementation to work a couple of limitations in the virtio-mmio
were needed to be overcomed, also the IVSHMEM imposed some extra challenges,
this section will try to clarify why some decisions were taken in order to
guide on how to proceed to bring this work to the upstream in the future.

Zephyr patched branch
*********************

This work was made on top of a patched zephyr branch, it is maintained indivually
from Zephyr upstream and OpenAMP staging branches, the west file of this repository
points to the following branch:

https://github.com/uLipe/zephyr/tree/feature/virtio-serial-device

The commit-id is: 2e3778ab0d5bcb36e2b38cabfc9adf5872a806da

Virtio-MMIO Hypervisorless
**************************

To make the virtio mmio driver to talk to the device side we needed to use the
hypervisor-less mode, which is responsible to create bounce buffers into a shared
memory area, and provides a custom IPI base API to generate interrupts between
the cores, since Zephyr provided the IPM over IVSHMEM the driver demo application
provides the IPI implementation on top of this IPM driver, check the
implementation in ``driver_side_demo/src/virtio_mmio_ivshmem.c``. Please notice
the device node of the ipc and ivshmem is hardcoded for this use case.

Virtio-MMIO memory address resolution
*************************************

The virtio-mmio driver provided in open-amp library is aimed to take
compile time absolute memory areas address, but the IVSHMEM resolves
its base address in runtime, to overcome that we touched in the zephyr
code of the virtio-mmio device driver and replaced the default behavior
of cfg and shared memory address, instead of absolute addresses they take
a form of memory offsets that are after summed into the IVSHMEM base address
in order to produce the correct final address.


Virtio-MMIO hypervisor-less memory allocation:
**********************************************
One challenging limitation of the virtio-mmio-hvl is in
respect of the VRINGS placement shared memory, it requires
an special linker section to be placed inside of the shared
memory area, a problematic step since IVSHMEM does not have
an fixed address, to overcome this limitation the DTS file
creates an extra region, offset based, memory area into
IVSHMEM space, allowing the virtio-mmio-hvl implementation
to place the VRINGS and map it into the IVSHMEM address space,
here the aspect of the device tree fragment for creating
the memory areas:

```
	reserved-memory {
		compatible = "reserved-memory";
		#address-cells = <1>;
		#size-cells = <1>;
		status = "okay";
		ranges;

		vmram_ivshmem0: vmram_ivshmem@800 {
			status = "okay";
			reg = < 0x800 0x80000 >;
			device_type = "memory";
			label = "vmram_ivshmem0";
		};

		vmram_ivshmem1: vmram_ivshmem@80800 {
			status = "okay";
			reg = < 0x80800 0x80000 >;
			device_type = "memory";
			label = "vmram_ivshmem1";
		};

		/* reserve this area for the vrings */
		vring_ivshmem: vmram_ivshmem@100800 {
			status = "okay";
			reg = < 0x100800 0x80000 >;
			device_type = "memory";
			label = "vring_ivshmem";
		};
	};

```
The areas declared above are summed into the IVSHMEM base address after
the Zephyr driver resolves it, then the addresses are initialized using
Zephyr heap structures, allowing to create a runtime VRING and Bounce
buffers allocation mechanism, the address generated from them when allocated
are already resolved and shareable to the device side during driver-device
negotiation phase.

Telling Zephyr we are using Virtio-MMIO with IVSHMEM
****************************************************

The additions above generated a couple of changes and extra functions inside
of the virtio-mmio zephyr device driver, in order to avoid breaking the current
existing implementation, a KConfig option was created in that driver, telling
some parts of the code of the driver will be overriden to meet specifics needs
of the IVSHMEM.

Device order initialization changing and its impacts
****************************************************

Since IVSHMEM driver is resolved in the POST_KERNEL stage of the Zephyr initialization
this impacted on the drivers that depend on it, so the virtio-mmio device driver now
gets initialized after IVSHMEM driver and its IPM driver, and after that, the virtio-mmio-serial
device driver is initialized then, the impact of that in the hvl use case is the console and
shell are not usable at the moment because they start earlier than those devices, instead
this demo overrides the printk output char function, making it available to play with the
demo.

