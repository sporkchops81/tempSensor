/*
Author: Eric Tsai
License: CC-BY-SA, https://creativecommons.org/licenses/by-sa/2.0/
Date: 9-1-2014
File: UberSensor.ino
This sketch is for a wired Arduino w/ RFM69 wireless transceiver
Sends sensor data (gas/smoke, flame, PIR, noise, temp/humidity) back
to gateway.  See OpenHAB configuration file.
1) Update encryption string "ENCRYPTKEY"
2)
*/


/* sensor
node = 12
device ID
4 = 1242 = human motion present or not
6 = 1262, 1263 = temperature, humidity

*/




//RFM69  --------------------------------------------------------------------------------------------------
#include <RFM69.h>
#include <SPI.h>
#define NODEID        12    //unique for each node on same network
#define NETWORKID     101  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "xxxxxxxxxxxxxxxx" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30 // max # of ms to wait for an ack
#define LED           9  // Moteinos have LEDs on D9
#define SERIAL_BAUD   9600  //must be 9600 for GPS, use whatever if no GPS

boolean debug = 0;

//struct for wireless data transmission
typedef struct {		
  int       nodeID; 		//node ID (1xx, 2xx, 3xx);  1xx = basement, 2xx = main floor, 3xx = outside
  int       deviceID;		//sensor ID (2, 3, 4, 5)
  unsigned long   var1_usl; 		//uptime in ms
  float     var2_float;   	//sensor data?
  float     var3_float;		//battery condition?
} Payload;
//create a payload
Payload theData;

char buff[20];
byte sendSize=0;
boolean requestACK = false;
RFM69 radio;

//end RFM69 ------------------------------------------

//temperature / humidity  =====================================
#include "DHT.h"
#define DHTPIN 7     			// digital pin we're connected to
#define DHTTYPE DHT11     // DHT 22  (AM2302) blue one
//#define DHTTYPE DHT21   // DHT 21 (AM2301) white one
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

// PIR sensor ================================================
int PirInput = 5;
int PIR_status = 0;
int PIR_reading = 0;
int PIR_reading_previous = 0;

// 4 = 1242 = human motion present or not
// 6 = 1262, 1263 = temperature, humidity

// timings
unsigned long pir_time;
//unsigned long pir_time_send;
unsigned long temperature_time;

void setup()
{
  Serial.begin(9600);          //  setup serial

  //RFM69-------------------------------------------
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  theData.nodeID = NODEID;  //this node id should be the same for all devices in this node
  //end RFM--------------------------------------------

  //temperature / humidity sensor
  dht.begin();
  
  //initialize times
  pir_time = millis();
  temperature_time = millis();
 
  //PIR sensor
  pinMode(PirInput, INPUT);
}

void loop()
{
  
  unsigned long time_passed = 0;
  
 //===================================================================
  //device #4
  //PIR
  
  //1 mean presence detected?
  PIR_reading = digitalRead(PirInput);
  //if (PIR_reading == 1)
  //Serial.println("PIR = 1");
  //else
  //Serial.println("PIR = 0");
  //send PIR sensor value only if presence is detected and the last time
  //presence was detected is over x miniutes ago.  Avoid excessive RFM sends
	if ((PIR_reading == 1) && ( ((millis() - pir_time)>60000)||( (millis() - pir_time)< 0)) ) //meaning there was movement
	{
		pir_time = millis();  //update gas_time_send with when sensor value last transmitted
		theData.deviceID = 4;
		theData.var1_usl = millis();
		theData.var2_float = 1111;
		theData.var3_float = 1112;		//null value;
		radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
    Serial.println("PIR detected RFM");
    delay(2000);
  }
  
  //===================================================================
  //device #6
  //temperature / humidity
  time_passed = millis() - temperature_time;
  if (time_passed < 0)
  {
	temperature_time = millis();
  }
  
  if (time_passed > 360000)
  {
    float h = dht.readHumidity();
    // Read temperature as Celsius
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit
    float f = dht.readTemperature(true);
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    Serial.print("Humidity=");
    Serial.print(h);
    Serial.print("   Temp=");
    Serial.println(f);
    temperature_time = millis();

	//send data
	theData.deviceID = 6;
	theData.var1_usl = millis();
	theData.var2_float = f;
	theData.var3_float = h;
	radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
  delay(1000);
	
  }
  //===================================================================

}//end loop

