# Arduino i2c reader/writer with terminal user interface

#### A handy tool using an Arduino to transfer an Intel HEX file to or from a desktop computer and read or write an i2c EEPROM.
#### The user inteface is by a terminal such as ExtraPutty or Terraterm. The Arduino displays a menu with commands to operate the functions which include:

#### Set start address.
#### Set end address.
#### Step through addresses and view and edit bytes in the EEPROM.
#### Read i2c EEPROM to Intel HEX format record, convert to XMODEM packets and send to computer to save as an Intel Hex file.
#### Load Intel HEX file from computer, using XMODEM recieve data packets to Arduino, convert to an Intel Hex record and write to i2c EEPROM.
#### Restart and auto detect EEPROM device address.



# To Do

#### The "int xmodemRecieve(void)" function starting at line 825 is imcomplete and has a strange bug where the return statements sometimes dont return back their values to the "void testRecieve(void)" function at line 972. - FIXED - Added a two second delay because Extraputty terminal is slow to respond.


#### A function that converts the 128 byte ASCII data field of the XMODEM packet to raw HEX values and then to Intel HEX records and then writes data to the i2 EEPROM is work in progress.

#### The user inteface is work in progress.



# Complete

#### System initialisation, i2c detect. 
#### EEPROM Editor for stepping through address and editing data at an address. 
#### Set start and End addresses.
#### Read i2c EEPROM, Convert to Intel HEX records.
#### Convert raw HEX data to ASCII and then to 128 byte blocks to assemble in to packets for XMODEM transmission.
