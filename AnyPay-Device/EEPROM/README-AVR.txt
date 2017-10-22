************************************************************************************
*					MANIFEST                                   *
************************************************************************************

Locate all files in your installation directory. 
The default installation directory is C:\Atmel\EmbeddedCrypto\DevLib\AVR\WinAVR-20090313


LIBRARY HEADER FILE(S)	
======================
	1. lib_Crypto.h		: Crypto core library header	

LIBRARY SUPPORT FILE(S)
======================
	1. lib_Support.c	: This file contains all the support functions for library

LIBRARY BINARY FILE(S)
======================
	1. lib_CM.a


This CryptoMemory library was compiled using WinAVR-20090313.
Optimization           : Os (Size)
CFLAGS                 : -Os -funsigned-char -funsigned-bitfields -fpack-struct 
                         -fshort-enums

INTERFACE FILES
===============
The interface files require re-implementation for specific targets

	1. ll_port.h		: Interface header file
	2. ll_port.c		: Interface implementation

Low level function to communicate with CryptoMemory depends on the hardware implementation.

Please refer to LibRef folder for description about command in CryptoMemory library.


