# UDP-Data-Logger
UDP Data Logger Client Program which can be used for extracting data from an Embedded Device

Background:

In an embedded device, it is useful to log the internal data for testing purpose. Usually the device
sends out a select number of data with a fixed sample internal. The physical communication medium can
be Serial RS232/RS485, USB, Ethernet, etc.

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

Client GUI:

![image](https://user-images.githubusercontent.com/70747761/166882748-8d30f961-ddb0-42ab-8095-1e819926de69.png)
