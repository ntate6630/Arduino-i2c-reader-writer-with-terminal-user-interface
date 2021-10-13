#include <stdlib.h>
#include <Wire.h>
#include "uCRC16XModemLib.h"
uCRC16XModemLib crc;
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A
#define MAXRETRANS 25
unsigned char dataBuffer[256] = { 't', 'e', 's', 't', '1', '2', '3', '4', 'H', 'e', 'l', 'l', 'o'};
int sourceSize = 256;
char receivedChar;
boolean newData = false, doOnce = false;
byte deviceAddress;
byte digit1, digit2, digit3, digit4, hexNum, highByte, lowByte;
unsigned int startAddress = 0x0000, endAddress = 0x0000;
word shiftBuffer;
byte lineOfData [24];

void setup()
{
    Wire.begin();                     
    Serial.begin(115200);
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
        Serial.print("\t\t5. Find device address\n\n\n");
        Serial.print("\t\t6. Test transmit\n\n\n");
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

void writeEEPROM(int endAddress, byte data, int deviceAddress)   // Write data to Serial EEPROM.
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
        
    Serial.print("\n device addr = ");
    Serial.print(deviceAddress, HEX);
    Serial.print(" eeprom addr = ");
    Serial.print(currentAddress, HEX);
    Serial.print(" eeprom RD data = ");
    Serial.print(rdata, HEX);   
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
                    shiftRightAndSplit();            // Shift digits left and split in to seperate digits.
                    Serial.print("\x1b[12;039f");   //Set cursor position. 
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
    boolean exitLoop = 0;
    Serial.print("\x1b[12;18f");  // Set cursor position. 
    Serial.print("\x1b[0K");    // Clear line.
    Serial.print("Read from device");
    while(exitLoop == 0)
    {
        if(Serial.available() > 0 )
        {
            receivedChar = Serial.read();
            if(receivedChar != 0x0D)    //Carriage return
            {
                writeHexFile();     // Reads serial EEPROM and generates HEX file data.
                // Transmit over serial port. - XMODEM code to follow.
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
                //Recieve from serial port. - XMODEM code to follow.
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
    byte byteCount;
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
                for(i = 0; i <= byteCount; i++ ) 
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
    byte checksum = 0;
    lineOfData[0] = ':';    // Start of line.
    lineOfData[1] = 0x10;   // Byte count 
    lineOfData[2] = 0x18;   // Address byte high.
    lineOfData[3] = 0x00;   // Address byte low.
    lineOfData[4] = 0x00;   // Record type.
    lineOfData[5] = 0x3E;   // Data byte 1. 
    lineOfData[6] = 0x55;   // Data byte 2.
    lineOfData[7] = 0xD3;   // Data byte 3. 
    lineOfData[8] = 0x00;   // Data byte 4.
    lineOfData[9] = 0x21;   // Data byte 5. 
    lineOfData[10] = 0x00;   // Data byte 6.
    lineOfData[11] = 0x20;   // Data byte 7. 
    lineOfData[12] = 0x01;   // Data byte 8.
    lineOfData[13] = 0xFF;   // Data byte 9. 
    lineOfData[14] = 0xFF;   // Data byte 10.
    lineOfData[15] = 0x0B;   // Data byte 11. 
    lineOfData[16] = 0x78;   // Data byte 12. 
    lineOfData[17] = 0xB1;   // Data byte 13.
    lineOfData[18] = 0x20;   // Data byte 14. 
    lineOfData[19] = 0xFB;   // Data byte 15.
    lineOfData[20] = 0x78;   // Data byte 16. 
    lineOfData[21] = 0x23;   // Data byte 17.
    generateChecksum();
}

void writeHexFile()
{
    boolean exitLoop = 0;
    byte i = 0;
    byte byteCount = 0x0F;
    unsigned int currentAddress;
    byte status = 0;
    currentAddress = startAddress;
    while(exitLoop == 0)
    {
        if(endAddress - startAddress < 0x0F)
            byteCount = endAddress - startAddress;
            
        highByte = currentAddress >> 8;     //Shift high byte in to low byte position.
        lowByte = currentAddress & 0xFF;
        lineOfData[0] = ':';                // Start of record.
        lineOfData[1] = byteCount;          // Number of data bytes.
        lineOfData[2] = highByte;           // High byte of start addrress.
        lineOfData[3] = lowByte;            // Low byte of start address.
        lineOfData[4] = 0x00;               // Record type.
        for(i = 0; i <= byteCount ; i++ )
        {
            lineOfData[5 + i] = readEEPROM(currentAddress, deviceAddress);
            currentAddress++;
            if(currentAddress > endAddress)
            {
                exitLoop = 1;
                Serial.print("\n Break = ");
                Serial.print(currentAddress, HEX);
                Serial.print("\n");
                break;
            } 
        } 
        generateChecksum();
        if(currentAddress > endAddress)
        {
            Serial.print("\n end of file sequence here \n");
            lineOfData[0] = ':';
            lineOfData[1] = 0x00;
            lineOfData[2] = 0x00;
            lineOfData[3] = 0x00;
            lineOfData[4] = 0x01;
            lineOfData[5] = 0xFF;
            for(i = 0; i < 6; i++ )
                Serial.print(lineOfData[i], HEX);   
        }
        delay(1000); 
        // put XMODEM transmit function here...
        // xmodemTransmit(unsigned char *source, int sourceSize)
    }
}

void generateChecksum()
{
    byte checksum = 0;
    byte byteCount = 0;
    byte i = 0;
    byteCount = lineOfData[1] + 4;
    for(i = 0; i <= byteCount; i++ )
        checksum = checksum + lineOfData[1 + i];
        
        Serial.print("\n pre checksum BC = ");
        Serial.print(byteCount, HEX);
        Serial.print("\n pre i = ");
        Serial.print(i, HEX);
    
    lineOfData[1 + i++] = checksum;
    lineOfData[i] =~ checksum;              // Ones compliment.
    lineOfData[i] ++;                       // Twos compliment.
        Serial.print("\n checksum result = ");
        Serial.print(lineOfData[i], HEX);
        Serial.print(" i after = ");
        Serial.print(i, HEX);
}

byte verifyData()
{
    byte checksum = 0;
    byte byteCount = 0;
    byte i;
    byte total = 0;
    byteCount = lineOfData[1] + 4;
    for(i = 0; i <= byteCount; i++ )
        checksum = checksum + lineOfData[1 + i]; 
        
        Serial.print("\n verify BC = ");
        Serial.print(byteCount, HEX);
        Serial.print("\n i = ");
        Serial.print(i, HEX);
        
    total = lineOfData[1 + i++] + checksum;
        Serial.print("\n verify checksum equal zero = ");
        Serial.print(total, HEX);
    if(total != 0)
        return 1;
    else
        return 0;         
}

int xmodemTransmit()
{
    unsigned char txbuff[134];
    unsigned char packetNumber = 1;
    int bufferSize, i, c, retry, crc = -1, len = 0;
    while(1) 
    {
        for( retry = 0; retry < 16; ++retry) 
        {
            delay(10000);
            while(Serial.available()) 
            {
                c = Serial.read();
            }
              //  Serial.print("\n ready = ");
              //  Serial.print(c, HEX);
                switch (c) 
                {
                    case 'C':
                        crc = 1;
                        goto start_transmission;
                    case NAK:
                        crc = 0;
                        goto start_transmission;
                    case CAN:
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

        while(1)
        {
            start_transmission:
            txbuff[0] = SOH; 
            bufferSize = 128;
            txbuff[1] = packetNumber;
            txbuff[2] = ~packetNumber;
            c = sourceSize - len;
            if(c > bufferSize) 
                c = bufferSize;
            if(c >= 0) 
            {                
                memset(&txbuff[3], 0, bufferSize);
                if(c == 0) 
                {
                    txbuff[3] = CTRLZ;
                }
                else 
                {
                    memcpy(&txbuff[3], &dataBuffer[len], c);
                    if(c < bufferSize) 
                        txbuff[3 + c] = CTRLZ;
                }
                if(crc) 
                {
                    unsigned int ccrc = crc16_ccitt(&txbuff[3], bufferSize);
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
                    txbuff[bufferSize +3] = ccks;
                }
                for(retry = 0; retry < MAXRETRANS; ++retry) 
                {
                    for(i = 0; i < bufferSize + 4 + (crc?1:0); ++i) 
                    {
                        Serial.write(txbuff[i]);
                    }
                    if(Serial.available()) 
                    {
                        c = Serial.read();
                    }    
                    if(c >= 0 ) 
                    {
                        switch(c) 
                        {
                            case ACK:
                                ++packetNumber;
                                len += bufferSize;
                                goto start_transmission;
                            case CAN:
                                if(Serial.available())
                                {
                                    c = Serial.read();
                                }
                                if(c == CAN) 
                                {
                                    Serial.write(ACK);
                                    flushInput();
                                    return -1;          // Canceled by remote 
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
                return -4; /* xmit error */ 
            }
            else 
            {
                for(retry = 0; retry < 10; ++retry) 
                {
                    Serial.write(EOT);
                    if(Serial.available())
                    {
                        c = Serial.read();
                    }
                    if (c == ACK) 
                        break;
                }
                flushInput();
                return (c == ACK)?len:-5;
            }
        }   
    }
}

unsigned int crc16_ccitt(unsigned char *buf, int len )
{
    unsigned int crc = 0;
        while( len-- ) 
        {
            int i;
            crc ^= *(char *)buf++ << 8;
            for( i = 0; i < 8; ++i ) 
            {
                if( crc & 0x8000 )
                    crc = (crc << 1) ^ 0x1021;
                else
                    crc = crc << 1;
            }
        }
    return crc;
}

static int check(int crc, unsigned char *buf, int sz)
{
    if(crc) 
    {
        unsigned int crc = crc16_ccitt(buf, sz);
        unsigned int tcrc = (buf[sz]<<8)+buf[sz+1];
        if(crc == tcrc)
            return 1;
    }
    else 
    {
        int i;
        unsigned char cks = 0;
        for(i = 0; i < sz; ++i) 
        {
            cks += buf[i];
        }
        if(cks == buf[sz])
            return 1;
        }
    return 0;
}

int xmodemReceive(unsigned char *dest, int destSize)
{
    unsigned char rxbuff[134]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
    unsigned char *p;
    int bufferSize, crc = 0;
    unsigned char trychar = 'C';
    unsigned char packetNumber = 1;
    int i, c, len = 0;
    int retry, retrans = MAXRETRANS;
    while(1) 
    {
        for(retry = 0; retry < 16; ++retry) 
        {
            if(trychar) 
                Serial.write(trychar);
            if(Serial.available())
            {  
                c = Serial.read();  
            }
            if(c >= 0) 
            {
                switch(c) 
                {
                    case SOH:
                        bufferSize = 128;
                        goto start_recv;
                    case STX:
                        bufferSize = 1024;
                        goto start_recv;
                    case EOT:
                        flushInput();
                        Serial.write(ACK);
                        return len;         // Normal end.
                    case CAN:
                        if(Serial.available())
                        {
                            c = Serial.read();
                        }        
                        if(c == CAN) 
                        {
                            flushInput();
                            Serial.write(ACK);
                            return -1;      // Canceled by remote.
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
        return -2;          // Sync error.

start_recv:
        if(trychar == 'C') 
            crc = 1;
        trychar = 0;
        p = rxbuff;
        *p++ = c;
        for(i = 0; i < (bufferSize + (crc?1:0) + 3); ++i) 
        {
            if(Serial.available())
            {
                Serial.read();
            }
            if(c < 0) 
                goto reject;
            *p++ = c;
        }
        if(rxbuff[1] == (unsigned char)(~rxbuff[2]) && (rxbuff[1] == packetNumber || rxbuff[1] == (unsigned char)packetNumber - 1) && check(crc, &rxbuff[3], bufferSize)) 
        {
            if(rxbuff[1] == packetNumber) 
            {
                register int count = destSize - len;
                if(count > bufferSize) 
                    count = bufferSize;
                if(count > 0) 
                {
                    memcpy (&dest[len], &rxbuff[3], count);
                    len += count;
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
                return -3;          // Too many retry errors.
            }
            Serial.write(ACK);
            continue;
        }
reject:
        flushInput();
        Serial.write(NAK);
    }
}

int testTransmit(void)
{
    int st;

    Serial.print("\nPrepare your terminal emulator to receive data now...\n");
          /* the following should be changed for your environment:
             0x30000 is the download address,
             12000 is the maximum size to be send from this address
          */
    st = xmodemTransmit();
    Serial.print(st, DEC);
    Serial.print("\n");
    if(st < 0) 
    {
        Serial.print("\nXmodem transmit error: "); 
        Serial.print(st, DEC);
        Serial.print("\n");
    }
    else  
    {
        Serial.print("\nXmodem successfully transmitted bytes");
        Serial.print(st, DEC);
        Serial.print("\n");
    }
    return 0;
    while(1)
    {
    }// Halt.
}