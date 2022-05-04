# UDP-Data-Logger
UDP Data Logger Client and Server Program which can be used for extracting data from an Embedded Device

Background:
In an embedded device, it is useful to log the internal data for testing purpose. Usually the device
sends out a select number of data with a fixed sample internal. The physical communication medium can
be Serial RS232/RS485, USB, Ethernet, etc.

Client

Description:
This program contains a UDP data logger client with GUI functionality. The embedded device acts as a
server which continuously sends out datagrams that the client receives/logs and stores in a CSV file.
The data content that the client expects to receive and the sample rate can easily be modified.
This program can also act as a server and the purpose for that is to test the client mode functionality
in case the embedded device is not available. As an example, the built-in server sends out three-phase
voltage waveform data once every 1ms. Configuration of the IP settings, viewing the communication
data/status/speed, opening the CSV file and event log file are included as part of the GUI functionality.
Retainment of the IP settings between each program start/end, setting default IP settings values and
detection of lost datagrams is also included. The logged CSV file can be opened for instance with Excel 
or Matlab to do the data analysis.

Tools: Microsoft Visual Studio 2022, .NET Framework 4.8, Windows Forms

Note: This program has only been tested on the Windows 10 operating system.

![image](https://user-images.githubusercontent.com/70747761/155664177-829b02b8-9262-435b-b5ec-3bdc761e353f.png)

Server

Description: 

The UdpDataLoggerServer folder contains a Code Composer Studio project that implements
a UDP data logger server that continuously sends out UDP data. The data is three phase
voltage waveforms. This server shall communicate with the client program located in
the UdpLoggerClient folder.

The IP address and port of the client and server are hardcoded and are the following:

Server: 192.168.1.3 52000
Client: 192.168.1.2 52000

This CCS project is intended to be run on the ICE (industrial communications engine) v2 development board.
The platform development kit for AM335X version 1.0.17 shall be used.

Tools:

HW

- ICE (industrial communications engine) v2 development board with the AM3359 processor
- 24V/1A power supply
- Micro USB cable
- Ethernet cable

SW

- Code Composer Studio (CCS) version 8.2
- Platform development kit for AM335X version 1.0.17 (containing the below directories)
pdk_am335x_1_0_17
xdctools_3_55_02_22_core
bios_6_76_03_01
gcc-arm-none-eabi-7-2018-q2-update
processor_sdk_rtos_am335x_6_03_00_106
ndk_3_61_01_01
ns_2_60_01_06
ti-cgt-pru_2.3.2
cg_xml_2.61.00
edma3_lld_2_12_05_30E
- Tera Term or some other serial terminal program

Instructions: 

Open Code Composer Studio and go to the Project Explorer.
In the project explorer, choose import CCS project. Then select the
UdpDataLoggerServer folder as the search directory.

![image](https://user-images.githubusercontent.com/70747761/166661223-8aa19c1d-4e4d-4f85-9808-9be0f3259449.png)
