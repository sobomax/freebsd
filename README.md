## About

This is ongoing effort to port FreeBSD to the Broadcom BCM5358U series of SoCs.

Some of the widespred hardware utilizing this SoC are:

 * Asus RT-N53 WiFi Router

## How to build

 $ make TARGET=mips TARGET_ARCH=mipsel KERNCONF=RT-N53 buildworld buildkernel

## How to boot

1. Setup TFPT server:

 tftp    dgram   udp     wait    root    /usr/libexec/tftpd      tftpd -l -s /usr/obj/mips.mipsel/usr/src/sys/RT-N53

2. Connect serial console
3. Reset or power-up, keep pressing Ctrl-C until you get into CFE prompt:

Decompressing...done


  CFE version 5.60.127.30 @VERSION_TYPE@ based on BBP 1.0.37 for BCM947XX (32bit,SP,LE)
  Build Date: ..  7. 29 20:42:09 CST 2011 (root@m9107761-virtual-machine)
  Copyright (C) 2000-2008 Broadcom Corporation.

  Init Arena
  Init Devs.
  Boot partition size = 131072(0x20000)
  Found a 8MB ST compatible serial flash
  et0: Broadcom BCM47XX 10/100/1000 Mbps Ethernet Controller 5.60.127.30 @VERSION_TYPE@
  CPU type 0x19749: 500MHz
  Tot mem: 32768 KBytes
  
  CFE mem:    0x80700000 - 0x8079FC10 (654352)
  Data:       0x80735A60 - 0x80738C90 (12848)
  BSS:        0x80738C90 - 0x80739C10 (3968)
  Heap:       0x80739C10 - 0x8079DC10 (409600)
  Stack:      0x8079DC10 - 0x8079FC10 (8192)
  Text:       0x80700000 - 0x80735A5C (219740)
  
  end of nvram_rescuegpio_init
  Device eth0:  hwaddr 54-04-A6-BA-B4-5C, ipaddr 192.168.1.1, mask 255.255.255.0
          gateway not set, nameserver not set
  Null Rescue Flag.
  end of nvram_rescuegpio_init
  Loader:raw Filesys:tftp Dev:eth0 File:: Options:(null)
  Loading: TFTP Server.
  Failed.
  Could not load :: Interrupted
  CFE> ^C
  CFE> ^C
  CFE>

4. Configure network interface (optional):

  CFE> ifconfig -addr=192.168.2.62 -mask=255.255.255.0 eth0
  Device eth0:  hwaddr 54-04-A6-BA-B4-5C, ipaddr 192.168.2.62, mask 255.255.255.0
          gateway not set, nameserver not set
  *** command status = 0

5. Load and boot kernel:

  CFE> boot -tftp -raw -addr=0x80800000 -max=0x770000 192.168.2.10:kernel.tramp.bin
  Loader:raw Filesys:tftp Dev:eth0 File:192.168.2.10:kernel.tramp.bin Options:(null)
  Loading: TFTP Client.
  .....TFTP_BLKLEN!!
  break!! last block!!
  .. 4115197 bytes read
  Entry at 0x807ffff0
  Closing network.
  Starting program at 0x807ffff0
  entry: mips_init()

