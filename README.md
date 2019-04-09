# About LDMicro
LDmicro is a ladder logic editor, simulator and compiler for 8-bit microcontrollers. It can generate native code for Atmel AVR and Microchip PIC16 CPUs from a ladder diagram.

# LDMicro for Linux
This is a Linux compatible version of the original LDMicro program for windows created by Jonathan Westhues, and ported to Linux using the Qt library.

GitHub repository of LDMicro for windows can be found [here](https://github.com/akshay-c/LDmicro).

# Instructions
Before modifying contents of this git account, kindly make sure that you do not add unnecessary files that are created during the make process. In order to do so ensure that you add appropriate command to the makefile and execute "make clean" script before uploading your changes to git.

# Building LDMicro for Linux
LDmicro for Linux is built using MinGW C++ compiler. 

### Building the Makefile
In order to generate the Makefile that can be used to compile the project, simply run the following commands from a terminal in the project directory:
```
cd /LDmicro/
mkdir build
cd build/
cmake ..
```

### Building LDMicro for Linux
Simply run `make` in the build directory of the project to compile.

_Note_: In order to compile the Makefile must first be generated (see [Building the Makefile](#building-the-makefile)).

Multiple Perl Scripts are executed during the build phase. In order to execute these scripts, to install the perl packages from the terminal (see [External package dependencies](#external-package-dependencies)).

## External package dependencies
The install commands for all the packages required to compile LDMicro for Linux are given below:

_Note_: Be sure to run `sudo apt-get update` and `sudo apt-get upgrade` before running the following commands

* Cmake: `sudo apt-get install cmake`
* GTK3: `sudo apt-get install libgtk-3-dev`
* MinGW: 
  ```
  sudo apt-get install gcc-mingw-w64
  sudo apt-get install g++-mingw-w64
  ```
* Perl: `sudo apt-get install perl`

## Running and testing LDMicro for Linux
### To run program in shell mode use the below command
`./LDmicro /c <.ld file to compile> <.hex destination file>`

### To run program in GUI mode use the below command
`sudo ./LDMicro`
