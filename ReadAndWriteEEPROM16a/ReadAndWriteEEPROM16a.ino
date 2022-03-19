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
unsigned int currentAddress, startAddress = 0x0000, endAddress = 0x0001, byteCounter;
word shiftBuffer;
byte lineOfData[272], state = 0, stage = 0;
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
            //    testDataForHexFile();    // Test hex file data.
            //    readHexFile();          // Read HEX file and write to serial EEPROM.
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

int readHexFile()
{
    byte i, count, byteCount;
    Serial1.print(lineOfData[0], HEX);
    Serial1.print(" ");
    if(lineOfData[0] == ':')              
    {
        byteCount = lineOfData[1];
        highByte = lineOfData[2];             // High byte of start address.
        lowByte = lineOfData[3];              // Low byte of start address.
        startAddress = highByte << 8;         // Combine high byte and low byte in 16 bit start address.
        startAddress = startAddress & 0xFF00;
        startAddress = startAddress | lowByte;
        Serial1.print(" Start Address = ");
        Serial1.print(startAddress, HEX);
        if(verifyData() == 0)                 // Verify the checksum.
        {
            Serial1.print("\nChecksum OK");
            Serial1.print("\nBytecount = ");
            Serial1.print(byteCount, HEX);
            Serial1.print("\n ");
            endAddress = startAddress;
            if(lineOfData[4] == 0x00)         // Record type is data.
            {
                for(i = 0; i < byteCount; i++ ) 
                {   
                    //                            writeEEPROM(endAddress, lineOfData[5 + i], deviceAddress);
                    Serial1.print("\nEnd address = ");
                    Serial1.print(endAddress, HEX);
                    Serial1.print("\nData = ");
                    endAddress++;
                    Serial1.print(lineOfData[5 + i], HEX);
                }
                return 0;                         // Indicate a data record is complete.
            }
            if(lineOfData[4] == 0x01 && lineOfData[5] == 0xFF)
            {
                Serial1.print(" EOF ");
                return 1;                         // Indicate end of file record.
            }
        }
        else
        {
            Serial1.print("\nChecksum error!");
            return -1;                        // Indicate checksum error.
        }
    }
    else return -2;
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
    if(stage == 0)                            // Stage 0 - create data record.
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
            lineOfData[4] = 0x00;                 // Indicates data record type. 
            rawHex = true;
            stage = 1;
        }
        return exitLoop = false;
    }  
    if(stage == 1)                                // Stage 1 - Read data from EEPROM and store in data field.                   
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
    if(stage == 5)                        // Stage 5 - create end of file record. 
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
            lineOfData[4] = 0x01;         // Indicates end of file record.
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
    if(c > 0x2F && c < 0x3A)
        return c - 0x30;
    if(c > 0x40 && c < 0x47)
        return c - 0x37;
    if(c > 0x60 && c < 0x67)
        return c - 0x57;       
}

byte generateChecksum()
{

    byte i, checksum = 0x00, byteCount;
    byteCount = lineOfData[1];
    for(i = 0; i < byteCount + 4; i++)
    {
        checksum = checksum + lineOfData[1 + i];
    }
    checksum =~ checksum;             // Ones compliment.
    checksum ++;                      // Twos compliment.
    return checksum;
}

byte verifyData()
{
    byte i, checksum = 0x00, total = 0x00, byteCount;
    Serial1.print(" checksum_entry ");
    byteCount = lineOfData[1];
    Serial1.print(" CS_BC = ");
    Serial1.print(byteCount, HEX);
    Serial1.print(" \n");
    for(i = 0; i < byteCount + 4; i++)
    {
        checksum = checksum + lineOfData[1 + i]; 
        Serial1.print(i, DEC);
        Serial1.print(" ");
        Serial1.print(checksum, HEX);
        Serial1.print(",");
    }
    i++;
    total = lineOfData[i] + checksum;
    Serial1.print("i = ");
    Serial1.print(i, DEC);
    Serial1.print(" T = ");
    Serial1.print(total, HEX);
    if(total == 0)
    {
        return 0;     // OK.
    }
    else
    { 
        return 1;     // NOT OK.
    }      
}

int xmodemTransmit()
{
    bool exitLoop, addToNextPacket = false;
    byte previousState;
    unsigned char txbuff[134], packetNumber = 1;
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
        return -2;            // No sync
        for(;;)
        {
            start_transmission:
            Serial1.print("\nCRC = ");
            Serial1.print(crc, DEC);
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
                if(crc) 
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

unsigned int crc16_ccitt(const unsigned char *buffer, int len )
{
    unsigned int crc = 0;
    while(len --) 
    {
        int i;
        crc ^= *(char *)buffer++ << 8;
        for(i = 0; i < 8; ++i) 
        {   
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}   

static int check(int crc, const unsigned char *buff, int sz)
{
    if(crc) 
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
    int bufferSize = 128, i = 0, j = 0, k = 0, c, counter = 0, crc = 0, retrans = MAXRETRANS, retry, getStatus = 0, byteCounter1 = 0;
    unsigned char rxbuff[134], packetNumber = 1, trychar = 'C';
    byte temp;
    delay(12000);
    state = 0;
    while(1) 
    {
        for(retry = 0; retry < 5; ++retry) 
        {   
            if(trychar)
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
                        j = 0;
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
                        counter = j;
                        return counter;               // Normal end. 
                    case CAN:
                        if(Serial.available()) 
                        {
                            c = Serial.read(); 
                            if(c == CAN) 
                            {
                                Serial.write(ACK);
                                flushInput();
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
        if(trychar == 'C') 
            crc = 1;  
        trychar = 0; 
        Serial1.print("\n CRC = ");
        Serial1.print(crc, HEX);
        Serial1.print(" \n"); 
        Serial1.print("firstbyte = ");
        Serial1.print(c, HEX);
        Serial1.print("\n");
        rxbuff[0] = c;
        for (i = 0;  i < (bufferSize + (crc ? 1 : 0) + 3); ++i) 
        {
            if(Serial.available())
                rxbuff[1 + i] = Serial.read();          
        }    
        if(rxbuff[1] == (unsigned char)(~rxbuff[2]) && (rxbuff[1] == packetNumber || rxbuff[1] == (unsigned char)packetNumber - 1) && check(crc, &rxbuff[3], bufferSize))
        {   
            if(rxbuff[1] == packetNumber)  
            { 
                Serial1.print(" Packet = ");
                Serial1.print(packetNumber, DEC);
                Serial1.print("\n");

                while(getStatus == 0)
                {
                    if(3 + j > bufferSize + 2)
                        goto get_next_packet;
                    if(state == 0)
                    {   
                        Serial1.print(" state = 0 ");                  
                        if(rxbuff[3 + j] == ':')
                        {
                            k = 0;
                            lineOfData[k] = rxbuff[3 + j];
                            Serial1.print(lineOfData[k], HEX);
                            Serial1.print("\n");
                            Serial1.print("J1 = ");
                            Serial1.print(j, DEC);
                            Serial1.print(", ");
                            Serial1.print("K = ");
                            Serial1.print(k, DEC); 
                            Serial1.print("\n");
                            j++;
                            k++;
                            state = 1;
                        }
                    }
                    if(3 + j > bufferSize + 2)                       
                        goto get_next_packet;
                    if(state == 1) 
                    {
                        Serial1.print(" state = 1 ");
                        Serial1.print("J = ");
                        Serial1.print(j, DEC);
                        Serial1.print(", ");
                        Serial1.print(rxbuff[3 + j], HEX);
                        lineOfData[k] = char2hex(rxbuff[3 + j]) << 4 & 0xF0;
                        j++;
                        Serial1.print(",");
                        Serial1.print(rxbuff[3 + j], HEX);
                        state = 2;
                    }  
                    if(3 + j > bufferSize + 2)  
                        goto get_next_packet;
                    if(state == 2)
                    {
                        Serial1.print(" state = 2 ");
                        Serial1.print("J = ");
                        Serial1.print(j, DEC);
                        Serial1.print(", "); 
                        temp = char2hex(rxbuff[3 + j]) & 0x0F;
                        lineOfData[k] = lineOfData[k] | temp;
                        Serial1.print(" D = ");
                        Serial1.print(lineOfData[k], HEX);
                        Serial1.print(" ");
                        Serial1.print("K = ");
                        Serial1.print(k, DEC);
                        Serial1.print("\n");
                        state = 3;
                    }
                    if(3 + j > bufferSize + 2)
                        goto get_next_packet;
                    if(state == 3)
                    {     
                        Serial1.print(" state = 3 ");  
                        j++;
                        Serial1.print(" J == ");
                        Serial1.print(j, DEC);
                        k++; 
                        Serial1.print(" K == ");
                        Serial1.print(k, DEC);
                        state = 4;
                    }
                    if(3 + j > bufferSize + 2)
                        goto get_next_packet;
                    if(state == 4)
                    {
                        Serial1.print(" state = 4 ");
                        if(rxbuff[3 + j] == 0x0D && rxbuff[3 + j + 1] == 0x0A)
                        {
                            Serial1.print("\n EOL = ");
                            Serial1.print(rxbuff[3 + j], HEX);
                            Serial1.print(" ");
                            Serial1.print(rxbuff[3 + j + 1], HEX);
                            Serial1.print("\nJ = ");
                            Serial1.print(j, DEC);
                            Serial1.print("\n");
                            j = j + 2;
                            state = 0;
                        }
                        else 
                        {
                            state = 1;
                            continue;
                        }
                    }
                    Serial1.print("X = ");
                    Serial1.print(lineOfData[0], HEX);
                    Serial1.print("\n ");
                    getStatus = readHexFile();              // Read HEX record and write data to i2c EEPROM.  
                    Serial1.print("getStatus = ");
                    Serial1.print(getStatus, DEC);
                    Serial1.print("\n");   
                }        
get_next_packet: 
                Serial1.print("state = ");
                Serial1.print(state, DEC);
                Serial1.print(" J_EOF = ");
                Serial1.print(j, DEC);
                Serial1.print("\n");      
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
                delay(100);
                return -3;                      // Too many retry errors.
            }
            Serial.write(ACK);
            Serial1.print(" ACK \n"); 
            continue;
        }  
reject:
        Serial1.print("CRC error\n");
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
