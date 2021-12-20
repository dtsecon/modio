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
(**i**)nput (**o**)utput. It supports modbus RTU and modbus TCP connections. Device and register   
information can be supplied as external structured configuration files and used to format the   
read data.  
 
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

 1. **libcofnig**. You can either install libconfig as distribution packages:  

    * `iibconfig9`   
    * `libconfig-dev`  

    or download compile and install from sources:  

    * `git clone https://github.com/hyperrealm/libconfig` - download the source
    * `cd libconfig`   
    * `autoreconf`   
    * `./configure`   
    * `make`   
    * `sudo make install`   

    for more details see: 
    * `https://github.com/hyperrealm/libconfig/blob/master/INSTALL`

 2. **libmodbus**. The latest stable release is available in many Linux distributions.  
    You can also download, compile and install from the source code :  

    * `git clone https://github.com/stephane/libmodbus` - download the source
    * `cd libmodbus`
    * `./autogen.sh`
    * `./configure --prefix=/usr/local/` - defalut installation prefix=/usr
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
    * `git clone https://github.com/dtsecon/modio`- download the source
    * `cd modio`
    * `./autoreconf `
    * `./configure` - default installation prefix=/usr/local
    * `make`
    * `make install`
    

CONFIGURATION
-------------

**modio** tool can read or write directly to a modbus register either defined as a register number  
or address. Register access info is given via a command line switch `--addr, -a`. The register  
data read from a device are printed on the shell standard output along with additional register  
meta data.  

	~$ modio -p192.168.2.104 -g -a5020 -t2 -o2 -r  
	reg: 5020 name: deviceUpTime address: 0x0003139c value: 103598s  

Device and register meta-data are not required for **modio** to read or write   
a register.

	~$ modio -p192.168.2.104 -g -a5020 -t2 -r -l2 -f6
	reg: 5020 address: 0x0003139c value: 105214


Register meta-data info is organized into configuration files with each file representing a specific   
device register list. Device Configration files are organized into two sections. The first section   
specifies the device meta-data...

	device =
	{
    	    manfc = "MOXA";
    	    type = "RIO";
    	    model = "IoLogik E1200";
    	    zba = 0;
	};

...while the second one specifies the registers' meta-data.

	regs =
	(
	    {
	        num = 5040;
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
	        num = 5020;
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

**modio** scans the directory `/usr/local/share/modio` for device register configuration files at run   
time. The user can create and add more device configuration files by following the aforementioned syntax   
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
--(z)ero           modbus zero based addressing (address|register - 1)
--(a)ddr    <val>| memory map address (default address 0x0)
        <reg_num>| register number (default 1) if -g has been specified
        <v,v,v,v>  comma separated values of addresses or registers e.g. -a0x40032,0x40101,0x4078
                   example: modio -p/dev/ttyS0 -a0x40032,0x40101,0x4078 -r -t3
--(r)ead           read data from memory
--(w)rite    <val> write data to address or register
--(l)en      <val> length of read count from address or register (default 1)
                   length is defined in words or registers and word size depends on register type
                   example: modio -p/dev/ttyS0 -a0x40078 -l3 -r -t3 reads 3 16bit registers
                   starting from address 0x40078
--re(g)_access     read (write) data from (to) register 
--reg_(t)ype <val> register type (0:COIL 1:INPUT_BIT 2:INPUT_REG 3:HOLDING, default 0)
--(f)ormat   <val> format print output (default value 2)
                   0:bin
                   1:hex
                   2:dec
                   3:ascii
                   4: dot ('.') separated bytes as dec
                   5: dot ('.') separated bytes as hex
                   6: high/low register words as dec
--reg_inf(o)  [id] print registers' meta data info of device id, if it is available
--(d)ev_info  [id] id is optional, if defined print registers' info for selected device otherwise
                   print list of supported devices
--r(e)ad_all  <id> read all registers' from device with <id> in the list of supported devices
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
	NUM   ADDRESS      NAME                                DESCRIPTION                  LEN RANGE      SCALE   ENGU         ACC
	5040  0x313b0      deviceName                                                       30             1.00                 R  
	5020  0x3139c      deviceUpTime                                                     2              1.00    s            R  
	5029  0x313a5      firmwareVersion                                                  2              1.00                 R  
	5031  0x313a7      firmwareBuildDate                                                2              1.00                 R  
	5027  0x313a3      lanIp                                                            2              1.00                 R  
	5024  0x313a0      lanMac                                                           3              1.00                 R  
	5000  0x31388      modelName                                                        10             1.00                 R  
	4144  0x1030       watchdogAlarmFlag                                                1              1.00                 R/W
	1000  0x103e8      DI_counterOverflowFlag              0:Normal,1:Overflow          16             1.00                 R  
	288   0x120        DI_counterOverflowFlagClear                                      16             1.00                 R/W
	272   0x110        DI_counterReset                     1: reset to initial value    16             1.00                 R/W
	256   0x100        DI_counterStatus                    0:STOP,1:START               16             1.00                 R  
	16    0x30010      DI_counterValue                     high/low word                32             1.00                 R  
	0     0x10000      DI_status                           0:OFF,1:ON                   16             1.00                 R  
	...
```
3. Read starting from modbus register at address 0x313b0 on modbus TCP server with ip address 192.168.2.104,    
   a 30 words long buffer and print the results as an ascii string:
```
	~$ modio -p192.168.2.104 -a0x313b0 -r -l30 -f3
	reg: 5040 address: 0x000313b0 value: iologik-E1212
```
4. Read the modbus INPUT register number 5020 on modbus TCP server with ip address 192.168.2.104, and print   
   the results using register meta-data info from device with id 2:
```
	~$ modio -p192.168.2.104 -g -a5020 -t2 -r -o2
	reg: 5020 name: deviceUpTime address: 0x0003139c value: 111048s
```
5. Read all registers from modbus server with ip address 192.168.2.104, using register meta-data info from   
   device with id 2:
```
	~$ modio -p192.168.2.104 -e2
	RIO MOXA IoLogik E1200:
	REG  NAME                                ADDRESS    VALUE   
	5040 deviceName                          0x000313b0 iologik-E1212
	5020 deviceUpTime                        0x0003139c 111461.00s
	5029 firmwareVersion                     0x000313a5 3.1.0.0
	5031 firmwareBuildDate                   0x000313a7 319489551.00
	5027 lanIp                               0x000313a3 192.168.2.104
	5024 lanMac                              0x000313a0 0.90.e8.8b.2d.5a
	5000 modelName                           0x00031388 E1212
	4144 watchdogAlarmFlag                   0x00001030 0
	1000 DI_counterOverflowFlag              0x000103e8 0
	1001 DI_counterOverflowFlag              0x000103e9 0
	1002 DI_counterOverflowFlag              0x000103ea 0
	1003 DI_counterOverflowFlag              0x000103eb 0
	1004 DI_counterOverflowFlag              0x000103ec 0
```


MAINTAINERS
-----------

 * Dimitris Oikonomou - dimitris.s.economou@gmail.com

Supporting organization:

 * inAccess - https://www.inaccess.com
