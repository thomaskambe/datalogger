#include <Ethernet.h>
#include <EthernetUdp.h> 
#include <SD.h>
#include <Wire.h>

byte mac[] = {0x98, 0x4F, 0xEE, 0x01, 0x5B, 0x00};  // mac adress of galileo
IPAddress ip(10,2,95,6);                            // ip address of galileo
unsigned int localPort = 5810;                      // local port to listen on
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];          // buffer to hold incoming packet,
EthernetUDP Udp;

const int DS1307 = 0x68;
const int redPin = 7;
const int grnPin = 6;
const int redPin2 = 9;
const int grnPin2 = 10;
const int a0Pin = 2;
const int a1Pin = 3;
const int a2Pin = 4;
const int shortOn = 250;
const int shortOff = 200;
const int longOff = 1000;
const int errorFlash = 500;
const int timeOut = 10000;

File dataFile;
char dataFileName[40];
byte second;
byte minute;
byte hour;
byte day;
byte year;
byte month;


// --------------------------------------------------
//   get the match number and increment
// --------------------------------------------------
int getMatchNum(void)
{
  File matchFile;
  char matchNumFile[] = "matchNum.txt";
  int match = 0;

  if (!SD.exists(matchNumFile))
  {
    matchFile = SD.open(matchNumFile, FILE_WRITE);
    if (matchFile)
    {
      matchFile.print(++match);
      matchFile.close();
    }
    else
    {
      return (0);
    }
  }
  else
  {
    matchFile = SD.open(matchNumFile, FILE_READ);
    if (matchFile)
    {
      char fileData = matchFile.read();
      while (fileData != -1)
      {
        match = match*10 + (fileData - '0');
        fileData = matchFile.read();
      }
      matchFile.close();

      SD.remove(matchNumFile);
      matchFile = SD.open(matchNumFile, FILE_WRITE);
      match += 1;
      matchFile.print(match);
      matchFile.close();
    }
    else
    {
      return (0);
    }
  }
  return (match);
}  // getMatchNum


// --------------------------------------------------
//   blink error lights
// --------------------------------------------------
void error()
{
  while(1)
  {
    digitalWrite(redPin, LOW);
    delay(shortOff);
    digitalWrite(redPin, HIGH);
    delay(shortOff);
  }
}  // error


// --------------------------------------------------
//   setup function
// --------------------------------------------------
void setup()
{
  int match;

  system("telnetd -l /bin/sh");   // open galileo for ethernet access
  Serial.begin(9600);
  Serial.println("begin");

  // set the date on the galileo using the RTC
  Wire.begin();
  readTime();
  sprintf(dataFileName, "date %02d%02d%02d%02d20%02d", month, day, hour, minute, year);
  system(dataFileName);

  // begin the UDP connections
  Ethernet.begin(mac,ip);  // robot
  //Ethernet.begin(mac);   // dhcp
  Udp.begin(localPort);

  // set the I/O pins
  pinMode(a0Pin, OUTPUT);     // A0
  pinMode(a1Pin, OUTPUT);     // A1
  pinMode(a2Pin, OUTPUT);     // A2
  pinMode(redPin, OUTPUT);    // red
  pinMode(grnPin, OUTPUT);    // green
  pinMode(redPin2, OUTPUT);   // red
  pinMode(grnPin2, OUTPUT);   // green
  digitalWrite(redPin, LOW);  // off
  digitalWrite(grnPin, LOW);  // off
}  //setup


// --------------------------------------------------
//   create data file and write headers
// --------------------------------------------------
int createDataFile(void)
{
  //if (!(match = getMatchNum())) error();
  //sprintf(dataFileName, "%08d.csv", match);  // no clock
  readTime();
  sprintf(dataFileName,"%02d_%02d-%02d-%02d.csv", day, hour, minute, second);
  dataFile = SD.open(dataFileName, FILE_WRITE);
  dataFile.print("time;tempLeftDrive4;tempLeftDrive3;tempRightDrive1;tempRightDrive2;tempLift0;tempCompressor6;");
  dataFile.print("currentLift;currentMotor2;currentMotor3;currentMotor4;");
  dataFile.println("currentMotor5;voltageRobot;liftEncoder");  // write the header to the file
  dataFile.print("0;0;0;0;0;0;0;0;0;0;0;0;0;0");  // write all zeros for the first line
  dataFile.flush();
  return (0);
}  // createDataFile


// --------------------------------------------------
//   main loop
// --------------------------------------------------
void loop()
{
  static int s = 0;
  static int ledState = 0;
  static unsigned long initialTime;
  static unsigned long writeTime;
  static unsigned long packetTime;
  static unsigned long grnTime = 0;
  static unsigned long redTime = 0;
  int packetSize;
  int i;
  static int adc[6] = {0,0,0,0,0,0};
  static char chan = 0;
  float temp;
  static unsigned long muxTime = 0;
  char tempString[40];

  switch(s)
  {

    case 0:   //wait for first packet
      packetSize = Udp.parsePacket();
      if (packetSize)
      {
        if (!createDataFile())
        {
          i = Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
          initialTime = millis();
          packetTime = initialTime;
          writeTime = 0;
          if (packetBuffer[0] != '#')
          {
            dataFile.print(";\r\n");
            dataFile.print(writeTime);
            sprintf(tempString, ";%d;%d;%d;%d;%d;%d;", adc[0], adc[1], adc[2], adc[3], adc[4], adc[5]);
            dataFile.print(tempString);
          }
          else
          {
            dataFile.print(";");
          }
          dataFile.write((uint8_t *)packetBuffer, i);
          dataFile.flush();
          redTime = initialTime + errorFlash;
          digitalWrite(redPin, HIGH);
          digitalWrite(redPin2, HIGH);
          s = 1;
        }
        else
        {
          s = 7;
        }
      } // if
      break;

    case 1:   // wait for packet
      packetSize = Udp.parsePacket();
      if (packetSize)
      {
        i = Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
        packetTime = millis();
        writeTime = packetTime - initialTime;
        if (packetBuffer[0] != '#')
        {
          dataFile.print(";\r\n");
          dataFile.print(writeTime);
          sprintf(tempString, ";%d;%d;%d;%d;%d;%d;", adc[0], adc[1], adc[2], adc[3], adc[4], adc[5]);
          dataFile.print(tempString);
        }
        else
        {
          dataFile.print(";");
        }
        dataFile.write((uint8_t *)packetBuffer, i);
        dataFile.flush();
        redTime = packetTime + errorFlash;
        digitalWrite(redPin, HIGH);
        digitalWrite(redPin2, HIGH);
      }
      if ((millis() - packetTime) > timeOut)
      {
        s = 3;
      }
      break;

    case 3:  // close
      dataFile.print("\r\n");
      dataFile.close();
      s = 0;
      break;

    case 7:   // error
      break;

  }  // switch s


  //led state machine

  switch(ledState)
  {
    case 0:
      if (millis() > redTime)
      {
        digitalWrite(redPin, LOW);
        digitalWrite(redPin2, LOW);
      }
      if ( millis() > grnTime)
      {
        digitalWrite(grnPin, HIGH);
        digitalWrite(grnPin2, HIGH);
        grnTime = millis() + shortOn;
        ledState = 1;
      }
      break;

    case 1:
      if (millis() > redTime)
      {
        digitalWrite(redPin, LOW);
        digitalWrite(redPin2, LOW);
      }
      if (millis() > grnTime)
      {
        digitalWrite(grnPin, LOW);
        digitalWrite(grnPin2, LOW);
        grnTime = millis() + shortOff;
        if (s == 0)
        {
          ledState = 3;
        }
        else
        {
          ledState = 2;
        }
      }
      break;

    case 2:
      if (millis() > redTime)
      {
        digitalWrite(redPin, LOW);
        digitalWrite(redPin2, LOW);
      }
      if ( millis() > grnTime)
      {
        digitalWrite(grnPin, HIGH);
        digitalWrite(grnPin2, HIGH);
        grnTime = millis() + shortOn;
        ledState = 3;
      }
      break;

    case 3:
      if (millis() > redTime)
      {
        digitalWrite(redPin, LOW);
        digitalWrite(redPin2, LOW);
      }
      if (millis() > grnTime)
      {
        digitalWrite(grnPin, LOW);
        digitalWrite(grnPin2, LOW);
        grnTime = millis() + longOff;
        if (s == 7)
        {
          ledState = 4;
        }
        else
        {
          ledState = 0;
        }
      }
      break;

    case 4:
      if (millis() > grnTime)
      {
        grnTime = millis() + shortOff;
        digitalWrite(grnPin, HIGH);
        digitalWrite(grnPin2, HIGH);
        digitalWrite(redPin, LOW);
        digitalWrite(redPin2, LOW);
        ledState = 5;
      }
      break;

    case 5:
      if (millis() > grnTime)
      {
        grnTime = millis() + shortOff;
        digitalWrite(grnPin, LOW);
        digitalWrite(grnPin2, LOW);
        digitalWrite(redPin, HIGH);
        digitalWrite(redPin2, HIGH);
        if (s == 7)
        {
          ledState = 4;
        }
        else
        {
          ledState = 3;
        }
      }
      break;   
    }  // led switch

  if (millis() > muxTime)
  {
    temp = (float)analogRead(0) * 0.488;
    adc[chan] = (int)temp;
    //Serial.println(adc[chan]);
    if (++chan > 5) 
    {
      chan = 0;
    }
    mux(chan);
    muxTime = millis() + 50;
  }
}  // loop



// --------------------------------------------------
//   switch the mux channels
// --------------------------------------------------
void mux(char chan) 
{
  switch(chan) 
  {
    case 0:   // sensor 0
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, LOW);
      break;
  
    case 1:   // sensor 1
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, LOW);
      break;
  
    case 2:   // sensor 2
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, HIGH);
      digitalWrite(a2Pin, LOW);
      break;
  
    case 3:   // sensor 3
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, HIGH);
      digitalWrite(a2Pin, LOW);
      break;
  
    case 4:   // sensor 4
      digitalWrite(a0Pin, LOW);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, HIGH);
      break;
      
    case 5:
      digitalWrite(a0Pin, HIGH);
      digitalWrite(a1Pin, LOW);
      digitalWrite(a2Pin, HIGH);
      break;
    
  } // switch

} // mux


// --------------------------------------------------
//   convert byte to decimal
// --------------------------------------------------
byte bcdToDec(byte val) 
{
  return ((val / 16 * 10) + (val % 16));
  
} // bcdToDec


// --------------------------------------------------
//   print the time from the RTC
// --------------------------------------------------
void printTime() 
{
  char buffer[3];
  const char* AMPM = 0;

  readTime();
  Serial.print("20");
  Serial.print(year);
  Serial.print("-");
  Serial.print(month);
  Serial.print("-");
  Serial.print(day);

  Serial.print(" ");
  if (hour > 12)
  {
    hour -= 12;
    AMPM = " PM";
  }
  else AMPM = " AM";
  Serial.print(hour);
  Serial.print(":");
  sprintf(buffer, "%02d", minute);
  Serial.print(buffer);
  Serial.println(AMPM);
  
} // printTime


// --------------------------------------------------
//   read the time from the RTC
// --------------------------------------------------
void readTime()
{
  Wire.beginTransmission(DS1307);
  Wire.write(byte(0));
  Wire.endTransmission();
  Wire.requestFrom(DS1307, 7);
  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read());
  Wire.read();
  day = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());

} // readTime

