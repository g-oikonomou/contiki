This example has been developed and tested with [6lbr](http://cetic.github.com/6lbr).

This guide contains instructions on how to build a 6LoWPAN-to-Internet gateway with 6lbr on a BeagleBone Black + CC2531 USB dongle.

Instructions are for BeagleBone Black running Debian

1. On the BBB

    1.  `sudo apt-get install bridge-utils`

        Not required for the setup used in this guide, but it will be required
        for SMART-BRIDGE mode

    2.  Download 6lbr

            git clone --recursive https://github.com/cetic/6lbr

    3.  `cd 6lbr/examples/6lbr`
    4.  `make all_native plugins tools`
    5.  `sudo make install`
    6.  Create `/etc/6lbr/6lbr.conf` with the content below

            MODE=ROUTER
            #MODE=SMART-BRIDGE
            #MODE=RPL-RELAY
            #MODE=FULL-TRANSPARENT-BRIDGE
            #MODE=NDP-ROUTER
            #MODE=6LR
            #MODE=RPL-ROOT

            RAW_ETH=1
            BRIDGE=0
            DEV_BRIDGE=br0
            DEV_TAP=tap0
            DEV_ETH=eth0
            RAW_ETH_FCS=1

            DEV_RADIO=/dev/ttyACM0
            BAUDRATE=115200

            LOG_LEVEL=3

    7.  6lbr will use channel 26 by default

            $ /usr/lib/6lbr/bin/nvm_tool --print /etc/6lbr/nvm.dat
            Reading nvm file '/etc/6lbr/nvm.dat'
            Channel : 26

            WSN network prefix : aaaa::
            WSN network prefix length : 64
            WSN IP address : aaaa::100
            WSN accept RA : True
            WSN IP address autoconf : True

            Eth network prefix : bbbb::
            Eth network prefix length : 64
            Eth IP address : bbbb::100
            Eth default router : ::
            Eth IP address autoconf : False

            Local address rewrite : True
            Smart Multi BR : False

            RA daemon : True
            RA router lifetime : 0
            RA maximum interval : 600
            RA minimum interval : 200
            RA minimum delay : 3
            RA PIO enabled : True
            RA prefix valid lifetime : 86400
            RA prefix preferred lifetime : 14400
            RA RIO enabled : True
            RA RIO lifetime : 1800

            RPL instance ID : 30
            RPL Preference : 0
            RPL version ID :  : 59
            RPL DIO interval doubling : 8
            RPL DIO minimum interval : 12
            RPL DIO redundancy : 10
            RPL default lifetime : 30
            RPL minimum rank increment : 256
            RPL lifetime unit : 256

            Webserver configuration page disabled : False

        To change to a different channel (e.g. 25)

            $ /usr/lib/6lbr/bin/nvm_tool --update --channel 25 /etc/6lbr/nvm.dat
            Reading nvm file '/etc/6lbr/nvm.dat'
            Channel : 25
            [...]

    8.  By default 6lbr will use PAN-ID 0xABCD

        To change it, edit `<6lbr-sources-dir>/examples/6lbr/project-conf.h`.
        Change the value of the `IEEE802154_CONF_PANID` macro

            #undef IEEE802154_CONF_PANID
            #define IEEE802154_CONF_PANID	0xABCD

        The Contiki CC2538 and CC26XX ports use 0x5449

2. On a linux or Mac OS X PC

    1.  Download our working repository from bitbucket:

            git clone --recursive https://<your-bitbucket-username>@bitbucket.org/antzougias/contiki-cc26xx.git

    2.  Install a recent version of SDCC as per the general CC2530 instructions
        in the Contiki wiki. Currently tested successfully with:

            $ sdcc -v
            SDCC : mcs51 3.4.1 #9068 (Sep 10 2014) (Mac OS X x86_64)

    3.  `cd <contiki-sources-dir>/examples/cc2530dk/cc2531-slip-radio`
    4.  `make`
    5.  Program the USB dongle with `cc2531-slip-radio.hex`

3. Back on the BBB

    1.  Connect the dongle to the BBB.

        You should see the two LEDs fade on/off in order. The dongle should appear as `/dev/ttyACM0`.

        Typing `dmesg` should show respective entries:

            $ dmesg
            [ 1366.311906] usb 1-1: new full-speed USB device number 3 using musb-hdrc
            [ 1366.647816] usb 1-1: ep0 maxpacket = 8
            [ 1366.654559] usb 1-1: skipped 4 descriptors after interface
            [ 1366.655833] usb 1-1: default language 0x0409
            [ 1366.663609] usb 1-1: udev 3, busnum 1, minor = 2
            [ 1366.663663] usb 1-1: New USB device found, idVendor=0451, idProduct=16a8
            [ 1366.663703] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
            [ 1366.663739] usb 1-1: Product: CC2531 USB Dongle
            [ 1366.663772] usb 1-1: Manufacturer: Texas Instruments
            [ 1366.663806] usb 1-1: SerialNumber: 00124B00015A755C
            [ 1366.665161] usb 1-1: usb_probe_device
            [ 1366.665213] usb 1-1: configuration #1 chosen from 1 choice
            [ 1366.666836] usb 1-1: adding 1-1:1.0 (config #1, interface 0)
            [ 1366.667469] cdc_acm 1-1:1.0: usb_probe_interface
            [ 1366.667526] cdc_acm 1-1:1.0: usb_probe_interface - got id
            [ 1366.667583] cdc_acm 1-1:1.0: This device cannot do calls on its own. It is not a modem.
            [ 1366.685171] cdc_acm 1-1:1.0: ttyACM0: USB ACM device
            [ 1366.689513] usb 1-1: adding 1-1:1.1 (config #1, interface 1)
            [ 1366.691001] hub 1-0:1.0: state 7 ports 1 chg 0000 evt 0002
            [ 1366.691099] hub 1-0:1.0: port 1 enable change, status 00000103

        You can also verify things with `lsusb`

            $ lsusb
            Bus 001 Device 002: ID 0451:16a8 Texas Instruments, Inc.
            Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
            Bus 002 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub

            $ sudo lsusb -v
            Bus 001 Device 002: ID 0451:16a8 Texas Instruments, Inc.
            Device Descriptor:
              bLength                18
              bDescriptorType         1
              bcdUSB               2.00
              bDeviceClass            2 Communications
              bDeviceSubClass         0
              bDeviceProtocol         0
              bMaxPacketSize0         8
              idVendor           0x0451 Texas Instruments, Inc.
              idProduct          0x16a8
              bcdDevice            0.00
              iManufacturer           1 Texas Instruments
              iProduct                2 CC2531 USB Dongle
              iSerial                 3 00124B00015A755C
              [...]

    2.  By default, 6lbr will configure `aaaa::/64` for the 6lowpan and
        `bbbb::/64` for the eth-side v6 network.

        It will send Router Advertisements for both prefixes over eth, so PCs
        connected to the same network segment should configure their routing
        tables and/or prefixes based on their OS and configuration.

        Changing to different prefixes will require configurations on 6lbr (via
        nvm-tool). Additionally, changing the 6lowpan prefix will also require
        configuration of the firmware for mesh nodes.

    3.  `sudo service 6lbr start`

4.  Test with Contiki-powered CC2538s or srf06-cc26xx on the same channel and
    the same PAN-ID. Make sure the following match:
    * Channel: The Contiki CC2538 port uses channel 25 by default. Either
      configure 6lbr to use 25 (as above), or change Contiki to use 26.
    * PAN-ID:  The Contiki CC2538 port uses 0x5449 by default. Either change
      6lbr to use 0x5449, or change Contiki to use 0xABCD.
    * RDC: The two Contiki ports as well as the CC2531 slip-radio example
      will use ContikiMAC by default, so if you are using unmodified sources
      you will not need to configure this explicitly.

5. On any PC connected to the same eth segment as the BBB

        ping6 aaaa::<address of mesh node>

    To see 6lowpan fragmentation in action, add option `-s` to `ping6`

        ping6 -s 100 aaaa::<address of mesh node>

    If there is no communication:

    `ifconfig`: Make sure the PC has an interface with an address in `bbbb::/64`

        $ ifconfig
        [...]
        en0: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST> mtu 1500
          options=10b<RXCSUM,TXCSUM,VLAN_HWTAGGING,AV>
          ether 3c:07:54:74:48:85
          inet6 fe80::3e07:54ff:fe74:4885%en0 prefixlen 64 scopeid 0x4
          inet 192.168.1.110 netmask 0xffffff00 broadcast 192.168.1.255
          inet6 bbbb::3e07:54ff:fe74:4885 prefixlen 64 autoconf
          inet6 bbbb::d518:4dc:99bd:c48e prefixlen 64 autoconf temporary
          nd6 options=1<PERFORMNUD>
          media: autoselect (10baseT/UTP <half-duplex>)
          status: active

    `netstat -rn`: Make sure the PC has a route to `aaaa::/64` via `bbbb::100`

        $ netstat -rn
        Routing tables
        [...]
        Internet6:
        Destination                             Gateway                         Flags         Netif Expire
        ::1                                     ::1                             UHL             lo0
        aaaa::/64                               bbbb::100                       UGSc            en0
        bbbb::/64                               link#4                          UC              en0
        bbbb::3e07:54ff:fe74:4885               3c:7:54:74:48:85                UHL             lo0
        bbbb::d518:4dc:99bd:c48e                3c:7:54:74:48:85                UHL             lo0
        [...]
