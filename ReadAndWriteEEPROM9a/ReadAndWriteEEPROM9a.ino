#include <stdlib.h>
#include <Wire.h>
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A
#define MAXRETRANS 25
char receivedChar;
bool newData = false, doOnce = false, rawHex;
byte deviceAddress, digit1, digit2, digit3, digit4, hexNum, highByte, lowByte;
unsigned int currentAddress, startAddress = 0x0000, endAddress = 0x0001, byteCount, byteCounter;
word shiftBuffer;
byte lineOfData[26], state = 0, stage = 0;
void setup()
{
    Wire.begin(); 
    Wire.setClock(400000);                    
    Serial.begin(115200);
    Serial1.begin(115200);    // Used for debug.
    while (!Serial) 
    {
                   // Wait for USB serial connection.
    }
    flushInput();
    initialise();
}

void flushInput()
{
    while(Serial.available() > 0)     //Flush the serial input buffer.
    {
        receivedChar = Serial.read(); 
    }
}

void loop()
{
    if(doOnce == false)
    {
        Serial.print("\n\t\t1. Set start address\n");
        Serial.print("\t\t2. Set end address\n");
        Serial.print("\t\t3. Read from device\n");
        Serial.print("\t\t4. Write to device\n");
        Serial.print("\t\t5. Find device address\n");
        Serial.print("\t\t6. Test transmit\n");
        Serial.print("\t\t7. Edit EEPROM\n");
        Serial.print("\t\t8. Test receive\n");
        doOnce = true;
    }
    if(Serial.available() > 0)
    {
        receivedChar = Serial.read();
        newData = true;
    }
    if(newData == true) 
    {
        newData = false;
        switch(receivedChar)
        {
            case '1':
                setStartAddress();
                break;
            case '2':
                setEndAddress();
              break;
            case '3':
                readDevice();
                break;
            case '4':
                writeDevice();
                break;
            case '5':
                Serial.print("Get EEPROM Address");
                initialise();
                doOnce = false;
                break;
            case '6':
                Serial.print("Test transmit");
                testTransmit();
                break;
            case '7':
                editEEPROM();
                break;
            case '8':
                testReceive();
                break;
        }
    }
}

void initialise()
{    
    Serial.print("\x1B[2J");      //Clear the screen.
    Serial.print("\x1b[01;01f");  //Set cursor position. 
    Serial.print("\x1B[33m");     //Set text colour to yellow.
    Serial.print("\n\t\tFinding i2c EEPROM address... ");
    deviceAddress = i2cScan();
    Serial.println(deviceAddress << 1, HEX);
    Serial.print("\n");
}

byte i2cScan()            //Find the 12c device address.
{
    byte error;
    for(deviceAddress = 0; deviceAddress < 256; deviceAddress++ )
    {
        // The i2c scan uses the return value of the Wire.endTransmisstion to see if a device did acknowledge to the address.
        Wire.beginTransmission(deviceAddress);
        error = Wire.endTransmission();
        if(error == 0)
            break;    //Exit the loop - Device address found

        if(error == 4)
        {
            Serial.println("Unknown error");
            break;
        }
    }
        delay(300);
        return deviceAddress;
}

void writeEEPROM(int endAddress, byte data, int deviceAddress)      // Write data to Serial EEPROM.
{
    int byteswritten = 0;
    Serial.print("\neeprom WR data = ");
    Serial.print(data, HEX);
    Serial.print(" eeprom addr = ");
    Serial.print(endAddress, HEX);
    Serial.print("\n device addr = ");
    Serial.print(deviceAddress, HEX);
    Wire.beginTransmission(deviceAddress);
    Wire.write((int)(endAddress >> 8));
    Wire.write((int)(endAddress & 0xFF));
    Wire.write(data);
    Wire.endTransmission();
    delay(6);
}

byte readEEPROM(unsigned int currentAddress, int deviceAddress)               // Read data from serial EEPROM.
{
    byte rdata;
    Wire.beginTransmission(deviceAddress);
    Wire.write((int)(currentAddress >> 8));
    Wire.write((int)(currentAddress & 0xFF));
    Wire.endTransmission(false);
    Wire.requestFrom(deviceAddress, 1);
    if (Wire.available())
        rdata = Wire.read(); 
    //Serial.print("\n device addr = ");
    //Serial.print(deviceAddress, HEX);
    //Serial.print(" eeprom addr = ");
    //Serial.print(currentAddress, HEX);
    //Serial.print(" eeprom RD data = ");
    //Serial.print(rdata, HEX);   
    return rdata;
}

void setStartAddress()
{
    boolean exitLoop = 0;
    Serial.print("\x1b[12;018f");  //Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("Set Start Address: ");
    Serial.print("0x");
    shiftBuffer = startAddress;
    shiftRightAndSplit();
    displayHexDigits();
    while(exitLoop == 0)
    {
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar != 0x0D)    //Carriage return 
            {
                shiftBuffer = shiftBuffer << 4;
                hexNum = int(receivedChar);
                if(hexNum > 47 && hexNum < 58 || hexNum > 64 && hexNum < 71 || hexNum > 96 && hexNum < 103)
                { 
                    if(hexNum > 64 && hexNum < 71)
                        hexNum = hexNum - 55;
                    if(hexNum > 96 && hexNum < 103)
                        hexNum = hexNum - 87;
                    hexNum = hexNum & 0x0F;
                    shiftBuffer = shiftBuffer | hexNum;
                    shiftRightAndSplit();             // Shift digits left and split in to seperate digits.
                    Serial.print("\x1b[12;039f");     //Set cursor position. 
                    displayHexDigits();            
                }
            }
            else
                break;
        }
    }
    Serial.print("\x1b[12;01f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("\x1b[18;018f");     // Set cursor position.
    Serial.print("Start Address: 0x");
    displayHexDigits();
    startAddress = shiftBuffer;
}

void setEndAddress()
{
    boolean exitLoop = 0;
    Serial.print("\x1b[12;018f");  //Set cursor position.
    Serial.print("\x1b[0K");    // Clear line. 
    Serial.print("Set End Address: ");
    Serial.print("0x");
    shiftBuffer = endAddress;
    shiftRightAndSplit();
    displayHexDigits();
    while(exitLoop == 0)
    {
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar != 0x0D)    //Carriage return 
            {
                shiftBuffer = shiftBuffer << 4;       //Shift digits in the buffer left towards most significant nibble to make way for next digit.
                hexNum = int(receivedChar);
                if(hexNum > 47 && hexNum < 58 || hexNum > 64 && hexNum < 71 || hexNum > 96 && hexNum < 103)
                { 
                    if(hexNum > 64 && hexNum < 71)
                        hexNum = hexNum - 55;
                    if(hexNum > 96 && hexNum < 103)
                        hexNum = hexNum - 87;
                    hexNum = hexNum & 0x0F;
                    shiftBuffer = shiftBuffer | hexNum;
                    shiftRightAndSplit();             // Shift right and split in to seperate digits.
                    Serial.print("\x1b[12;037f");     //Set cursor position. 
                    displayHexDigits();            
                }
            }
            else
                break;
        }
    }
    Serial.print("\x1b[12;01f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("\x1b[19;020f");     // Set cursor position.
    Serial.print("End Address: 0x");
    displayHexDigits();
    endAddress = shiftBuffer;
}

void readDevice()
{
    bool exitLoop = false;
    Serial.print("\x1b[12;18f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("Read from device");
    currentAddress = startAddress;
    byteCounter = endAddress - currentAddress;
    state = 0;
    stage = 0;
    exitLoop = 0;
    while(exitLoop == false)
    {
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar != 0x0D)    //Carriage return
            {
                Serial.print("\n");
                Serial.print(state, DEC);
                Serial.print("\t\t");
                Serial.print(currentAddress, HEX);
                exitLoop = writeHexFile(exitLoop);     // Reads serial EEPROM and generates HEX file data. 
                Serial.print("\t\t\t");
                Serial.print(lineOfData[state], HEX);
                Serial.print("\t\tBC = ");
                Serial.print(byteCounter, DEC);
                Serial.print("\t\t EL = ");
                Serial.print(exitLoop, HEX);
                Serial.print("\t\t Stage = ");
                Serial.print(stage, HEX);
                state++;
            }
            else
                break;
        }
    }
    Serial.print("\x1b[12;01f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
}

void writeDevice()
{
    boolean exitLoop = 0;
    Serial.print("\x1b[12;18f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("Write to device");
    while(exitLoop == 0)
    {
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar != 0x0D)    // Carriage return.
            {
                //Recieve from serial port. - XMODEM code to follow. ******************************
                testDataForHexFile();    // Test hex file data.
                readHexFile();          // Read HEX file and write to serial EEPROM.
            }
            else
                break;
        }
    }
    Serial.print("\x1b[12;01f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
}

void shiftRightAndSplit()
{
    digit1 = shiftBuffer >> 12;
    digit1 = digit1 & 0x0F;
    digit2 = shiftBuffer >> 8;
    digit2 = digit2 & 0x0F;
    digit3 = shiftBuffer >> 4;
    digit3 = digit3 & 0x0F;
    digit4 = shiftBuffer;
    digit4 = digit4 & 0x0F; 
}

void displayHexDigits()
{
    Serial.print(digit1, HEX);
    Serial.print(digit2, HEX);
    Serial.print(digit3, HEX);
    Serial.print(digit4, HEX);
}

void readHexFile()
{
    byte i;
    if(lineOfData[0] == ':')              
    {
        byteCount = lineOfData[1];
        highByte = lineOfData[2];             // High byte of start address.
        lowByte = lineOfData[3];              // Low byte of start address.
        startAddress = highByte << 8;         // Combine high byte and low byte in 16 bit start address.
        startAddress = startAddress & 0xFF00;
        startAddress = startAddress | lowByte;
        if(verifyData() == 0)                 // Verify the checksum.
        {
            Serial.print("\nChecksum OK");
            Serial.print("\nBytecount = ");
            Serial.print(byteCount, HEX);
            Serial.print("\n ");
            endAddress = startAddress;
            if(lineOfData[4] == 0x00)         // Record type is data.
            {
                for(i = 0; i < byteCount; i++ ) 
                {   
                    writeEEPROM(endAddress, lineOfData[5 + i], deviceAddress);
                    Serial.print("\nEnd address = ");
                    Serial.print(endAddress, HEX);
                    Serial.print("\nData = ");
                    endAddress++;
                    Serial.print(lineOfData[5 + i], HEX);
                }
            }
        }
        else
            Serial.print("\nChecksum error!");
        if(lineOfData[4] == 0x01)             // Record type is end of file.
        {
            if(lineOfData[5] == 0xFF)
                Serial.print("EOF");
        }   
    }
}    

void testDataForHexFile()
{
    lineOfData[0] = ':';    // Start of line.
    lineOfData[1] = 0x10;   // Byte count 
    lineOfData[2] = 0x12;   // Address byte high.
    lineOfData[3] = 0x34;   // Address byte low.
    lineOfData[4] = 0x00;   // Record type.
    lineOfData[5] = 0x54;   // Data byte 1. 
    lineOfData[6] = 0x68;   // Data byte 2.
    lineOfData[7] = 0x69;   // Data byte 3. 
    lineOfData[8] = 0x73;   // Data byte 4.
    lineOfData[9] = 0x20;   // Data byte 5.  
    lineOfData[10] = 0x70;   // Data byte 6.
    lineOfData[11] = 0x61;   // Data byte 7. 
    lineOfData[12] = 0x72;   // Data byte 8.
    lineOfData[13] = 0x74;   // Data byte 9. 
    lineOfData[14] = 0x20;   // Data byte 10.
    lineOfData[15] = 0x69;   // Data byte 11. 
    lineOfData[16] = 0x73;   // Data byte 12. 
    lineOfData[17] = 0x20;   // Data byte 13.
    lineOfData[18] = 0x69;   // Data byte 14. 
    lineOfData[19] = 0x6E;   // Data byte 15.
    lineOfData[20] = 0x20;   // Data byte 16. 
    lineOfData[21] = 0x23;   // Data byte 17.
    generateChecksum();
}

byte writeHexFile(bool exitLoop)
{
    if(byteCounter > 0x10)
        byteCounter = 0x10;
    highByte = currentAddress >> 8;           //Shift high byte in to low byte position.
    lowByte = currentAddress & 0xFF;
    if(stage == 0)
    {   
        if(state == 0)
        {
            lineOfData[0] = 0x3A;                  // Start of record - ASCII ':' 
            rawHex = false;
        }
        if(state == 1)
        {
            lineOfData[1] = byteCounter;
            rawHex = true;
        }
        if(state == 2)
        {
            lineOfData[2] = highByte;             // High byte of start addrress. 
            rawHex = true;
        }
        if(state == 3)
        {
            lineOfData[3] = lowByte;              // Low byte of start address. 
            rawHex = true;
        }
        if(state == 4)
        {
            lineOfData[4] = 0x00;                 // Data record type. 
            rawHex = true;
            stage = 1;
        }
        return exitLoop = false;
    }  
    if(stage == 1)  
    { 
        if(byteCounter > 0)
        {
            lineOfData[state] = readEEPROM(currentAddress, deviceAddress);
            byteCounter--;
            currentAddress++;
            rawHex = true;
            return exitLoop = false;
        }
        else
        {
         //   Serial.print(" CS ");
            lineOfData[state] = generateChecksum();
            stage = 2;
            byteCounter = endAddress - currentAddress;
            rawHex = true;
            return exitLoop = false;
        }
    }
    if(stage == 2)
    {
        lineOfData[state] = 0x0D;   // Carriage return.
        stage = 3;
        rawHex = false;
        return exitLoop = false;
    }
    if(stage == 3)
    {
        lineOfData[state] = 0x0A;     // Line feed.
        stage = 4;
        rawHex = false;
        return exitLoop = false;
    }
    if(stage == 4)
    {
        if(currentAddress + 1 > endAddress)
        {
            stage = 5;
            state = 0;
            return exitLoop = false;
        }
        if(currentAddress <= endAddress)
        {
            stage = 0;
            state = 0;
            return exitLoop = false;
        }
    }
    if(stage == 5)
    {   
        if(state == 0)
        {
            lineOfData[0] = 0x3A;         // Start of record - ASCII ':'
            rawHex = false;
        }
        if(state == 1)
        {
            lineOfData[1] = 0x00;
            rawHex = true;
        }
        if(state == 2)
        {
            lineOfData[2] = 0x00;
            rawHex = true;
        }
        if(state == 3)
        {
            lineOfData[3] = 0x00;
            rawHex = true;
        }
        if(state == 4)
        {
            lineOfData[4] = 0x01;
            rawHex = true;
        }
        if(state == 5)
        {
            //       Serial.print(" CS ");
            lineOfData[5] = generateChecksum();
            stage = 6;
            rawHex = true;
            return exitLoop = false;
        }
        return exitLoop = false;    
    }  
    if(stage == 6)
    {
        lineOfData[state] = 0x0D;     // Carriage Return.
        rawHex = false;
        stage = 7;
        return exitLoop = false;
    }
    if(stage == 7)
    {
        lineOfData[state] = 0x0A;     // Line feed.
        rawHex = false;
        return exitLoop = true;
    }
}

byte hex2char(byte c)
{
    byte x;
    c = c & 0x0F;
    if(c >= 0 && c <= 9)
        x = c + 0x30;
    if(c >= 0x0A && c <= 0x0F)
        x = c + 0x37;
    return x;
}

byte char2hex(byte c)
{
    byte x;
    if(c >= 0x30 && c <= 0x39)
        x = c - 0x30;
    if(c >= 0x41 && c <= 0x46)
        x = c - 0x37;
    if(c >= 61 && c <= 66)
        x = c - 57;
    return x;       
}

byte generateChecksum()
{

    byte i, checksum = 0x00, byteCount;
    byteCount = lineOfData[1];
    for(i = 0; i < byteCount + 4; i++ )
    {
        checksum = checksum + lineOfData[1 + i];
    }
    checksum =~ checksum;             // Ones compliment.
    checksum ++;                      // Twos compliment.
    return checksum;
}

byte verifyData()
{
    byte i, byteCount = 0, checksum = 0, total = 0;
    byteCount = lineOfData[1];
    for(i = 0; i < byteCount + 4; i++ )
        checksum = checksum + lineOfData[1 + i]; 
    i++;
    total = lineOfData[i] + checksum;
    if(total != 0)
    {
        return 1;     // NOT OK.
    }
    else
    { 
        return 0;     // OK.
    }      
}

int xmodemTransmit()
{
    bool exitLoop, addToNextPacket = false;
    byte previousState;
    unsigned char txbuff[133], packetNumber = 1;
    int bufferSize, i, c, retry, crc = -1;
    for(;;) 
    {
        for(retry = 0; retry < 16; ++retry) 
        {
            delay(12000);
            while(Serial.available()) 
            {
                c = Serial.read();
            }
                switch (c) 
                {
                    case 'C':
                        crc = 1;
                        currentAddress = startAddress;
                        byteCounter = endAddress - currentAddress;
                        exitLoop = false;
                        state = 0;
                        stage = 0;
                        goto start_transmission;
                    case NAK:
                        crc = 0;
                        currentAddress = startAddress;
                        byteCounter = endAddress - currentAddress;
                        exitLoop = false;
                        state = 0;
                        stage = 0;
                        goto start_transmission;
                    case CAN:
                        delay(100);
                        while(Serial.available())
                        {
                            c = Serial.read();
                        } 
                        if(c == CAN)
                        {
                            Serial.write(ACK);
                            flushInput();
                            return -1;        // canceled by remote.
                        }
                        break;
                    default:
                        break;
                }   
        }
        Serial.write(CAN);
        Serial.write(CAN);
        Serial.write(CAN);
        flushInput();
        return -2;        // No sync
        for(;;)
        {
            start_transmission:
            txbuff[0] = SOH; 
            bufferSize = 128;
            txbuff[1] = packetNumber;
            txbuff[2] = ~packetNumber;
            c = (endAddress - startAddress);  
            if(c > bufferSize) 
                c = bufferSize;
            if(c >= 0 && exitLoop == false) 
            {          
                for(i = 0; i < bufferSize; i++)     
                    txbuff[3 + i] = 0;
                if(c == 0)
                    txbuff[3] = CTRLZ;
                else
                {  
                    i = 0;
                    while(i < bufferSize)
                    {   
                        if(addToNextPacket == true)
                        {
                            txbuff[3 + i] = hex2char(lineOfData[previousState]);
                            //state++;
                            i++;
                            addToNextPacket = false;
                        }
  
                        if(exitLoop == false)
                        {
                            exitLoop = writeHexFile(exitLoop);
                            if(rawHex == false)
                                txbuff[3 + i] = lineOfData[state];
                            if(rawHex == true)
                            {
                                txbuff[3 + i] = hex2char(lineOfData[state] >> 4);
                                if(i >= 127)
                                {
                                    previousState = state;
                                    addToNextPacket = true;
                                }
                                else
                                {
                                    i++;
                                    txbuff[3 + i] = hex2char(lineOfData[state]);
                                }
                            }    
                            state++;  
                        }
                        else
                            txbuff[3 + i] = CTRLZ; 
                        i++;                  
                    }
                }
                if(crc == 1) 
                {
                    unsigned short ccrc = crc16_ccitt(&txbuff[3], bufferSize);
                    txbuff[bufferSize + 3] = (ccrc >> 8) & 0xFF;
                    txbuff[bufferSize + 4] = ccrc & 0xFF;
                }
                else 
                {
                    unsigned char ccks = 0;
                    for(i = 3; i < bufferSize + 3; ++i) 
                    {
                        ccks += txbuff[i];
                    }
                    txbuff[bufferSize + 3] = ccks;
                }
                for(retry = 0; retry < MAXRETRANS; ++retry) 
                {
                    for(i = 0; i < bufferSize + 4 + (crc?1:0); ++i) 
                    { 
                        Serial.write(txbuff[i]);
                    }
                    delay(100);
                    if(Serial.available()) 
                    {
                        c = Serial.read();
                    }    
                    if(c >= 0) 
                    {
                        switch(c) 
                        {
                            case ACK:
                                ++packetNumber; 
                                goto start_transmission;
                            case CAN:
                                delay(100);
                                if(Serial.available())
                                {
                                    c = Serial.read();
                                }
                                if(c == CAN) 
                                {
                                    Serial.write(ACK);
                                    flushInput();
                                    return -1;          // Canceled by remote. 
                                }
                                break;
                            case NAK:
                            default:
                                break;
                        }  
                    }
                } 
                Serial.write(CAN);
                Serial.write(CAN);
                Serial.write(CAN);
                flushInput();
                return -4;                              // Transmit error. 
            }
            else 
            {
                for(retry = 0; retry < 10; ++retry) 
                {
                    Serial.write(EOT);
                    Serial1.print(" EOT \n");
                    delay(100);
                    if(Serial.available())
                    {
                        c = Serial.read();
                    }
                    if(c == ACK)
                        break;
                }
                flushInput();
                return (c == ACK)?(currentAddress - startAddress) : -5;    // No ACK after EOT.
            }
        }   
    }
}

short crc16_ccitt(const unsigned char *buffer, int len )
{
    unsigned short crc = 0;
    while(len != 0) 
    {
        crc  = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= *buffer;
        crc ^= (unsigned char)(crc & 0xFF) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xFF) << 4) << 1;
        buffer++;
        len--;
    }
    return crc;
}

static int check(int crc, const unsigned char *buff, int sz)
{
    if(crc == 1) 
    {
        unsigned short crc = crc16_ccitt(buff, sz);
        unsigned short tcrc = (buff[sz] << 8) + buff[sz + 1];
        if(crc == tcrc)
            return 1;
    }
    else 
    {
        int i;
        unsigned char cks = 0;
        for(i = 0; i < sz; ++i) 
        {
            cks += buff[i];
        }
        if(cks == buff[sz])
            return 1;    
    }  
    return 0;
}

int xmodemReceive(void)
{
    int bufferSize = 128, crc = 0, i, c, counter = 0, retrans = MAXRETRANS, retry;
    unsigned char rxbuff[133], packetNumber = 1, trychar = 'C', destination[136];
    delay(12000);
    while(1) 
    {
        for(retry = 0; retry < 16; ++retry) 
        {   
            if(trychar == 'C')
            {
                Serial.write(trychar);
                Serial1.print("try = ");
                Serial1.print(trychar);
                Serial1.print(" \n");
            }
            delay(1000);
            if(Serial.available())
            {
                c = Serial.read();
                switch (c)
                {
                    case SOH:
                        Serial1.print("SOH");
                        Serial1.print(" \n");
                        bufferSize = 128;
                        goto start_reception;
                    case STX:   
                        flushInput();
                        Serial.write(CAN);      // Abort.
                        delay(2000);
                        Serial1.print("\n XMODEM 1K not used \n");
                        return -4;              // XMODEM 1K not used.
                    case EOT:
                        flushInput();
                        Serial.write(ACK);
                        Serial1.print("EOT");
                        Serial1.print("\n");
                        delay(100);
                        return counter;               // Normal end. 
                    case CAN:
                        if(Serial.available()) 
                        {
                            c = Serial.read(); 
                            if(c == CAN) 
                            {
                                flushInput();
                                Serial.write(ACK);
                                Serial1.print("Canceled by remote");
                                Serial1.print(" \n");
                                delay(100);
                                return -1;                // Canceled by remote. 
                            }
                        }
                        break;
                    default:
                        break;
                }  
            }                
        }   
        if(trychar == 'C')
        {
            trychar = NAK;
            continue;
        }   
        flushInput();
        Serial.write(CAN);
        Serial.write(CAN);
        Serial.write(CAN);
        Serial1.print("Error \n");
        return -2;                                      // Sync error. 
start_reception:   
        rxbuff[0] = SOH; 
        if(trychar == 'C')
            crc = 1;      
        trychar = 0;  
        Serial1.print("\n CRC = ");
        Serial1.print(crc, HEX);
        Serial1.print(" \n");  
        for(i = 0; i < (bufferSize + (crc?1:0) + 3); ++i) 
        {
            if(Serial.available())
            {
                c = Serial.read();
                if(c < 0)
                    goto reject;
                rxbuff[1 + i] = c;     
            }      
        }   
        if(rxbuff[1] == (unsigned char)(~rxbuff[2]) && (rxbuff[1] == packetNumber || rxbuff[1] == (unsigned char)packetNumber - 1) && check(crc, &rxbuff[3], bufferSize) == 0)
        {
            if(rxbuff[1] == packetNumber)  
            { 
                Serial1.print(" Packet = ");
                Serial1.print(packetNumber, DEC);
                Serial1.print("\n");
                for(i = 0; i < bufferSize; i++)
                {
                    destination[i] = rxbuff[3 + i];
                    Serial1.print(" ");
                    Serial1.print(destination[i]);
                    counter++;
                }    
                ++packetNumber;
                retrans = MAXRETRANS + 1;
            }    
            if(--retrans <= 0) 
            {
                flushInput();
                Serial.write(CAN);
                Serial.write(CAN);
                Serial.write(CAN);
                Serial1.print("Error too many retries \n");
                return -3;                      // Too many retry errors.
            }
            Serial.write(ACK);
            Serial1.print(" ACK \n");
            continue;
        }    
reject:
        flushInput();
        Serial.write(NAK);   
    }    
}

void testTransmit(void)
{
    int st;
    Serial.print("\n Prepare your terminal emulator to receive data now...\n");
    st = xmodemTransmit();
    Serial.print("\n");
    if(st < 0) 
    {
        Serial.print("\n Xmodem transmit error: "); 
        Serial.print(st, DEC);
        Serial.print("\n");
    }
    else  
    {
        Serial.print("\n Xmodem successfully transmitted ");
        Serial.print(st, DEC);
        Serial.print(" bytes \n");
    }
}

void testReceive(void)
{
    int st;
    Serial.print("\n Send data using the xmodem protocol from your terminal emulator now...\n");
    st = xmodemReceive();
    Serial.print("\n\n\n\n");
    if(st < 0)
    { 
        Serial.print(" Xmodem receive error: status: ");
        Serial.print(st, DEC);
        Serial.print("\n");
        Serial1.print("Error status = \n");
        Serial1.print(st, DEC);
    }
    else  
    {
        Serial.print(" Xmodem successfully received ");
        Serial.print(st, DEC);
        Serial1.print("\n bytes = ");
        Serial1.print(st, DEC);
    }
}

void editEEPROM()
{
    byte data, dataDigit1, dataDigit2, shiftBuff, i;
    boolean exitLoop = 0, editMode = 0;
    Serial.print("\x1b[18;018f");  //Set cursor position.
    Serial.print("\x1b[0K");    // Clear line. 
    shiftBuffer = startAddress;
    shiftRightAndSplit();
    displayHexDigits();
    do{
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar == '+')
            {
                startAddress ++;
                editMode = 0;
            }
            if(receivedChar == '-')
            {
                startAddress --;
                editMode = 0;
            }
            if(receivedChar > 47 && receivedChar < 58 || receivedChar > 64 && receivedChar < 71 || receivedChar > 96 && receivedChar < 103)
            {
                editMode = 1;
                shiftBuff = shiftBuff << 4;
                data = int(receivedChar);
                if(data > 64 && data < 71)
                    data = data - 55;
                if(data > 96 && data < 103)
                    data = data - 87;
                data = data & 0x0F;
                shiftBuff = shiftBuff | data;
                dataDigit1 = shiftBuff >> 4;
                dataDigit1 = dataDigit1 & 0x0F;
                dataDigit2 = shiftBuff;
                dataDigit2 = dataDigit2 & 0x0F;
                Serial.print("\x1b[18;028f");
                Serial.print(dataDigit1, HEX);
                Serial.print(dataDigit2, HEX); 
                if(i > 0)
                {
                    i = 0;
                    writeEEPROM(startAddress, shiftBuff, deviceAddress);
                }
                i++;
            }  
            if(editMode == 0)
            {        
                i = 0;   
                if(receivedChar != 0x0D)    //Carriage return 
                {
                    Serial.print("\x1b[18;018f");  //Set cursor position.
                    Serial.print("\x1b[0K");    // Clear line. 
                    shiftBuffer = startAddress;
                    shiftRightAndSplit();
                    displayHexDigits();
                    data = readEEPROM(startAddress, deviceAddress);
                    shiftBuff = data;
                    dataDigit1 = shiftBuff >> 4;
                    dataDigit1 = dataDigit1 & 0x0F;
                    dataDigit2 = shiftBuff;
                    dataDigit2 = dataDigit2 & 0x0F; 
                    Serial.print("\x1b[18;028f");
                    Serial.print(dataDigit1, HEX);
                    Serial.print(dataDigit2, HEX);           
                }
                else
                    break;
            }
        }
    }while(exitLoop == 0);
}
