# UDP-Data-Logger
UDP Data Logger Client and Server Program which can be used for extracting data an Embedded Device

Background:
In an embedded device, it is useful to log the internal data for testing purpose. Usually the device
sends out a select number of data with a fixed sample internal. The physical communication medium can
be Serial RS422/RS485, USB, Ethernet, etc.

Description:
This program contains a UDP data logger client with GUI functionality. The embedded device acts as a
server which continuousky sends out datagrams that the client receives/logs and stores in a CSV file.
The data content that the client expects to receive and the sample rate can easily be modified.
This program can also act as a server and the purpose is to test the client mode functionality
in case the embedded device is not available. As an example, the built-in server sends out three-phase
voltage waveform data once every 1ms. Configuration of the IP settings, viewing the communication
data/status/speed, opening the CSV file and event log file are included as part of the GUI functionality.
Retainment of the IP settings between each program start/end, setting default IP settings values and
detection of lost datagrams is also included. The logged CSV file can be opened with Excel or Matlab
to do the analysis.

Tools:
Microsoft Visual Studio 2022, .NET Framework 4.8
This program has only been tested on the Windows 10 operating system.
