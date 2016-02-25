## About

This is ongoing effort to port FreeBSD to the Broadcom 47xx series of SoCs.

## How to build

 $ make TARGET=mips TARGET_ARCH=mipsel KERNCONF=BCM471x buildworld buildkernel

## How to boot

1. Setup TFPT server:

 tftp    dgram   udp     wait    root    /usr/libexec/tftpd      tftpd -l -s /usr/obj/mips.mipsel/usr/src/sys/BCM471x

2. Connect serial console
3. Reset or power-up, keep pressing Ctrl-C until you get into CFE prompt:

 CFE>

4. Configure network interface (optional):

  CFE> xxx

5. Load and boot kernel
