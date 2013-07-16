#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <math.h>
#include <OneWire.h>

int BH1750address = 0x23;
byte buff[2];

// for yeelink api
#define APIKEY         "8a782063a9940ed271852daa4e4f8703" // replace your yeelink api key here
#define DEVICEID       328 // replace your device ID
#define SENSORID       409 // replace your sensor ID
OneWire  ds(5);  // on pin 5
// assign a MAC address for the ethernet controller.
byte mac[] = { 0x00, 0x1D, 0x72, 0x82, 0x35, 0x9D};
// initialize the library instance:
EthernetClient client;
char server[] = "api.yeelink.net";   // name address for yeelink API
//IPAddress server(202,136,60,231);      // numeric IP for api.yeelink.net

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 30*1000; // delay between 2 datapoints, 30s


void setup() {
  Wire.begin();
  // start serial port:
  Serial.begin(57600);
  // start the Ethernet connection with DHCP:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    for(;;)
      ;
  }
  else {
    Serial.println("Ethernet configuration OK");
  }
}

void loop() {
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    // read sensor data, replace with your code
    float sensorReading = readLightSensor();////////////////////////////////////////////////////////////////////////////////////////////
    //send data to server
    sendData(sensorReading);////////////////////////////////////////////////////////////////////////////////////////////////////////
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void sendData(float thisData) {/////////////////////////////////////////////////////////////////////////////////////////////////////////
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("POST /v1.0/device/");
    client.print(DEVICEID);
    client.print("/sensor/");
    client.print(SENSORID);
    client.print("/datapoints");
    client.println(" HTTP/1.1");
    client.println("Host: api.yeelink.net");
    client.print("Accept: *");
    client.print("/");
    client.println("*");
    client.print("U-ApiKey: ");
    client.println(APIKEY);
    client.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    // 8 bytes for {"value":} + number of digits of the data:
    int thisLength = 10+5;//getLength(thisData);//////////////////////////////////////////////////////////////////////
    client.println(thisLength);
    Serial.println(thisLength);

    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.print("{\"value\":");
    client.print(thisData);
    client.println("}");
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
   // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}

// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:
int getLength(int someValue) {/////////////////////////////////////////////////////////////////////////////////////////////////////
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten,
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

///////////////////////////////////////////////////////////////////////////
// get data from light sensor
// you can replace this code for your sensor
float readLightSensor()///////////////////////////////////////////////////////////////////////////////////////////////////
{byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  for ( ;!ds.search(addr);)
  {
   ds.reset_search();
    delay(250);
  }
  
  
  /*if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
   // Serial.println();
    ds.reset_search();
    delay(250);
    return 0;
  }*/
  
 

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 0;
  }
  //Serial.println();
 
  // the first ROM byte indicates which chip

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
  //delay(11000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
 // Serial.print(present,HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  //  Serial.print(data[i], HEX);
   // Serial.print(" ");
  }
 // Serial.print(" CRC=");
 // Serial.print(OneWire::crc8(data, 8), HEX);
 // Serial.println();

  // convert the data to actual temperature

  unsigned int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  //Serial.print("  Temperature = ");
 // Serial.print(celsius);
 // Serial.print(" Celsius, ");
 // Serial.print(fahrenheit);
  //Serial.println(" Fahrenheit");
  Serial.println(celsius);
  return celsius;///////////////////////////////////////
  
}


