# Arduino i2c reader/writer with terminal user interface

#### A handy tool using an Arduino to transfer an Intel HEX file to or from a desktop computer and read or write an i2c EEPROM.
#### The user inteface is by a terminal such as ExtraPutty or Terraterm. The Arduino displays a menu with commands to operate the functions which include:

#### Set start address.
#### Set end address.
#### Step through addresses and view and edit bytes in the EEPROM.
#### Read EEPROM to a HEX file.
#### Load HEX file and write to an EEPROM.
#### Restart and auto detect EEPROM device address.



# To Do

#### The "int xmodemRecieve(void)" function starting at line 825 is imcomplete and has a strange bug where the return statements sometimes dont return back their values to the "void testRecieve(void)" function at line 972.
#### Fixed - Added a two second delay because Extraputty terminal is slow to respond.


#### A function that converts the 128 byte ASCII data field to Intel HEX records and then writes data to the i2c EEPROM is work in progress.

#### The user inteface is not finished yet.
