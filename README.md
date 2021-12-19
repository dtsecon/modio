CONTENTS OF THIS FILE
---------------------

 * Introduction
 * Requirements
 * Installation
 * Configuration
 * Maintainers


INTRODUCTION
------------

**modio** is a generic tool for read and write modbus registers. modio stands for 
**mod**bus **i**nput **o**utput. It supports modbus RTU and modbus TCP connections. 
Device and register information can be supplied as external structured configuration 
files and used to format the read data.
 
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



MAINTAINERS
-----------

 * Dimitris Oikonomou - dimitris.s.economou@gmail.com

Supporting organization:

 * inAccess - https://www.inaccess.com
