CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Requirements
 * Installation
 * Configuration
 * Usage
 * Maintainers


INTRODUCTION
------------

**modio** is a generic tool for read and write modbus registers. **modio** stands for (**mod**)bus   
(**i**)nput (**o**)utput. It supports modbus RTU and modbus TCP connections. Modbus register   
data can be presented in binary, decimal, hexadecimal and ascii format. Additional device and   
register information can be supplied as external structured configuration files which can be   
used to format and enrich the modbus printout data.  
 
 * For the description of modio tool visit   
   https://github.com/dtsecon/modio

 * To submit bug reports and feature suggestions, or to track changes visit:   
   https://github.com/dtsecon/modio


REQUIREMENTS
------------
**modio** requires the following external libraries in order get compiled and run:  

 * **libconfig**, a simple library for processing structured configuration files   
   (https://hyperrealm.github.io/libconfig/)
 * **libmodbus**, a free software library to send/receive data according to the Modbus   
   protocol. This library is written in C and supports RTU (serial) and TCP (Ethernet)    
   communications. (https://libmodbus.org/)
 * **hashmap**, a generic C hashmap implementation with light-weight pre-processor macros   
   which provide a templated type-safe interface (https://github.com/tidwall/hashmap.c)


INSTALLATION
------------

 1. **libcofnig**. You can either install libconfig as distribution packages...

    * `iibconfig9`   
    * `libconfig-dev`  

    ...or download compile and install from sources: 

    * `git clone https://github.com/hyperrealm/libconfig` - download the source
    * `cd libconfig`   
    * `autoreconf`   
    * `./configure`   
    * `make`   
    * `sudo make install`   

    for more details read:
    * `https://github.com/hyperrealm/libconfig/blob/master/INSTALL`

 2. **libmodbus**. The latest stable release is available in many Linux distributions.  
    You can also download, compile and install from the source code: 

    * `git clone https://github.com/stephane/libmodbus` - download the source
    * `cd libmodbus`
    * `./autogen.sh`
    * `./configure --prefix=/usr/local/` - defalut installation `prefix=/usr`
    * `make`
    * `sudo make install`

 3. **hashmap**. To build and install on your host system, follow these easy steps:
    * `git clone https://github.com/DavidLeeds/hashmap.git` - download the source
    * `mkdir build-hashmap && cd build-hashmap` - create a build directory outside the source tree
    * `cmake ../hashmap` - run CMake to setup the build
    * `make` - compile the code
    * `make test` - run the unit tests (if enabled)
    * `sudo make install` - _OPTIONAL_ install the library on this system

 4. **modio**. Build and install modio:
    * `git clone https://github.com/dtsecon/modio` - download the source
    * `cd modio`
    * `./autogen.sh `
    * `./configure` - default installation prefix=/usr/local
    * `make`
    * `make install`
    

CONFIGURATION
-------------

**modio** tool can read or write directly to a modbus register either defined as a register number  
or address. Register access info is given via a command line switch `--reg (-g)`. The register  
data read from a device are printed on the shell standard output along with additional register  
meta data.  

	~$ modio -p192.168.2.104 -g35021 -o2 -r   
	reg: 35021 name: deviceUpTime address: 0x0003139c value: 103598s  

Device and register meta-data are not required for **modio** to read or write   
a register.

	~$ modio -p192.168.2.104 -a -g5020 -t2 -r -l2 -f6
	reg: 5020 address: 0x0003139c value: 105214


Register meta-data info is organized into configuration files with each file representing a specific   
device register list. Device Configration files are organized into two sections. The first section   
specifies the device meta-data...

	device =
	{
	    manfc = "MOXA";
	    type = "RIO";
	    model = "IoLogik E1200";
	    zba = 1;
	};

...while the second one specifies the registers' meta-data.

	regs =
	(
	    {
	        num = 35041;
	        addr = 0x0;
	        len = 30;
	        type = 2;
	        name = "deviceName";
	        descr = "";
	        range = "";
	        scale = 1.0;
	        engu = "";
	        access = "R";
	        print = 3;
	    },
	    {
	        num = 35021;
	        addr = 0x0;
	        type = 2;
	        len = 2;
	        name = "deviceUpTime";
	        descr = "";
	        range = "";
	        scale = 1.0;
	        engu = "s";
	        access = "R";
	        print = 6;
	    }
	)

The register meta-data fields are specified below:

	FIELD   DESCRIPTION                     TYPE   
	num:    register number                 (integer)   
	addr:   register address                (integer preferable in hex format)   
	type:   register type                   (0:COIL, 1:INPUT_BIT, 2:INPUT_REG, 3:HOLDING_REG)   
	name:   register name                   (string)   
	descr:  register description            (string)   
	range:  register value range            (string)   
	scale:  register value scale factor     (float)   
	engu:   register value engineering unit (string)   
	access: register access                 (string) e.g: R|W|RW   
	print:  register print format           (0:BIN 1:HEX 2:DEC 3:ASC 4:BFD 5:BFX 6:HLO)   

* All fields must be defined and honor the field type   
* `scale` is used by modio to calculate the register value when `-o <id>` switch is used   
* `type` is used by modio to select register access type without `-t <type>` switch   
* `print` is used by modio to print register as: 
  - binary (BIN), 
  - hex (HEX), 
  - decimal (DEC), 
  - ASCII (ASC), 
  - byte decimal (BFD),
  - byte hex (BFX),
  - high low word (HLO)

**modio** scans the directories `/usr/local/share/modio` and `$HOME/.modio` for device register   
configuration files at run time. All verified configuration files distributed with the **modio**   
source are copied in `/usr/local/share/modio` directory. The user can also create and add more    
device configuration files under `$HOME/.modio` directory, following the aforementioned syntax   
rules. 


USAGE
-----
```
Usage: modio [OPTIONS]...
--(p)ort     <val> device port can be either a file or an IP address (default: /dev/ttyUSB0)
                   example: /dev/tty<PORT>, 192.0.12.3, 192.168.1.2:1502 (default TCP port 502)
--baud       <val> serial port baud rate (default 9600)
--parity     <val> serial port parity (N:none O:odd E:even M:mark default N)
--sbit       <val> serial port stop bit (default 1)
--dbit       <val> serial port data bits (default 8)
--dev_(i)d   <val> modbus slave device id (default 1)
--(z)ero           disable modbus zero based addressing (address = register - type offset)
                   example: address = 35021(reg_num) - 30000(type_offset) = 5021
--re(g)     <val>| register number (default number 0x1)
        <address>| register address (default 0) if -a has been specified
        <v,v,v,v>  comma separated values of addresses or registers e.g. -a0x40032,0x40101,0x4078
                   example: modio -p/dev/ttyS0 -a0x40032,0x40101,0x4078 -r -t3
--(r)ead           read data from memory
--(w)rite    <val> write data to address or register
--(l)en      <val> length of read count from address or register (default 1)
                   length is defined in words or registers and word size depends on register type
                   example: modio -p/dev/ttyS0 -a0x40078 -l3 -r -t3 reads 3 16bit registers
                   starting from address 0x40078
--reg_(a)ddress    read (write) data from (to) register address 
--reg_(t)ype <val> register type (0:COIL 1:INPUT_BIT 2:INPUT_REG 3:HOLDING, default 0)
--(f)ormat   <val> format print output (default value 2)
                   0: bin
                   1: hex
                   2: dec
                   3: ascii
                   4: dot ('.') separated bytes as dec
                   5: dot ('.') separated bytes as hex
                   6: high/low register words as dec
--reg_inf(o)  [id] print registers' meta data info of device id, if it is available
--(d)ev_info  [id] id is optional, if defined print registers' info for selected device otherwise
                   print list of supported devices
--r(e)ad_all  <id> read all registers' from device with <id> in the list of supported devices
--debug      <val> print debug messages
--(h)elp           print usage
```

Examples:

1. Print the list with supported devices:
```
	~$ modio -d
	Supported devices:
	NUM TYPE         MANUFACTURER MODEL           REGS
	1   UPS          ADELE        CBI2801224A     27  
	2   RIO          MOXA         IoLogik E1200   24  
```
2. Print the list of registers of supported device:
```
	~$ modio -d2
	MOXA IoLogik E1200 RIO
	NUM    ADDRESS      NAME                                DESCRIPTION                  LEN RANGE      SCALE   ENGU         ACC
	35041  0x313b0      deviceName                                                       30             1.00                 R  
	35021  0x3139c      deviceUpTime                                                     2              1.00    s            R  
	35030  0x313a5      firmwareVersion                                                  2              1.00                 R  
	35032  0x313a7      firmwareBuildDate                                                2              1.00                 R  
	35028  0x313a3      lanIp                                                            2              1.00                 R  
	35025  0x313a0      lanMac                                                           3              1.00                 R  
	35001  0x31388      modelName                                                        10             1.00                 R  
	4145   0x1030       watchdogAlarmFlag                                                1              1.00                 R/W
	11001  0x103e8      DI_counterOverflowFlag              0:Normal,1:Overflow          16             1.00                 R  
	289    0x120        DI_counterOverflowFlagClear                                      16             1.00                 R/W
	273    0x110        DI_counterReset                     1: reset to initial value    16             1.00                 R/W
	257    0x100        DI_counterStatus                    0:STOP,1:START               16             1.00                 R  
	17     0x30010      DI_counterValue                     high/low word                32             1.00                 R  
	1      0x10000      DI_status                           0:OFF,1:ON                   16             1.00                 R  
	...
```
3. Read starting from modbus register number 35041 on modbus TCP server with ip address 192.168.2.104,    
   a 30 words long buffer and print the results as an ascii string:
```
	~$ modio -p192.168.2.104 -g35041 -r -l30 -f3
	reg: 35041 address: 0x000313b0 value: iologik-E1212
```
4. Read the modbus INPUT register address 5020 on modbus TCP server with ip address 192.168.2.104, and print   
   the results using register meta-data info from device with id 2:
```
	~$ modio -p192.168.2.104 -a -g5020 -t2 -r -o2
	reg: 35021 name: deviceUpTime address: 0x0003139c value: 111048s
```
5. Read starting from modbus INPUT register address 5024 on modbus TCP server with ip address 192.168.2.104, and print   
   the results as dot ('.') separated bytes in hex format:
```
	~$ modio -p192.168.2.104 -a -g5024 -t2 -r -l3 -f5
	reg: 5024 address: 0x000313a0 value: 0.90.e8.8b.2d.5a
```
6. Read all registers from modbus server with ip address 192.168.2.104, using register meta-data info from   
   device with id 2:
```
	~$ modio -p192.168.2.104 -e2
	RIO MOXA IoLogik E1200:
	REG   NAME                                ADDRESS    VALUE   
	35041 deviceName                          0x000313b0 iologik-E1212
	35021 deviceUpTime                        0x0003139c 111461.00s
	35030 firmwareVersion                     0x000313a5 3.1.0.0
	35032 firmwareBuildDate                   0x000313a7 319489551.00
	35028 lanIp                               0x000313a3 192.168.2.104
	35025 lanMac                              0x000313a0 0.90.e8.8b.2d.5a
	35001 modelName                           0x00031388 E1212
	04145 watchdogAlarmFlag                   0x00001030 0
	11001 DI_counterOverflowFlag              0x000103e8 0
	11002 DI_counterOverflowFlag              0x000103e9 0
	11003 DI_counterOverflowFlag              0x000103ea 0
	11004 DI_counterOverflowFlag              0x000103eb 0
	11005 DI_counterOverflowFlag              0x000103ec 0
```
7. Read eight registers starting from modbus COIL register number 1 from modbus server with ip address 192.168.2.104
```
	~$ modio -p192.168.2.104 -g1 -l8 -r
	reg: 00001 address: 0x00000000 value: 0
	reg: 00002 address: 0x00000001 value: 0
	reg: 00003 address: 0x00000002 value: 0
	reg: 00004 address: 0x00000003 value: 0
	reg: 00005 address: 0x00000004 value: 0
	reg: 00006 address: 0x00000005 value: 0
	reg: 00007 address: 0x00000006 value: 0
	reg: 00008 address: 0x00000007 value: 0
```
8. Write eight registers at modbus server with ip address 192.168.2.104, starting from modbus COIL register number 1 with    
   the value 1 and then read them all:
```
	~$ modio -p192.168.2.104 -g1 -l8 -w1 -r
	reg: 00001 address: 0x00000000 value: 1
	reg: 00002 address: 0x00000001 value: 1
	reg: 00003 address: 0x00000002 value: 1
	reg: 00004 address: 0x00000003 value: 1
	reg: 00005 address: 0x00000004 value: 1
	reg: 00006 address: 0x00000005 value: 1
	reg: 00007 address: 0x00000006 value: 1
	reg: 00008 address: 0x00000007 value: 1
```

MAINTAINERS
-----------

 * Dimitris Oikonomou - dimitris.s.economou@gmail.com

Supporting organization:

 * inAccess - https://www.inaccess.com
