/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
/*-------- temp drivers--------*/
#include <OneWire.h>
OneWire  ds(2);  // on pin 2 --- PIN:D4 GPIO:2 PIN TYPE:TXD1 Board:ESP8266 (a 4.7K resistor is necessary)
/*-----------temp drivers END -------*/

/* Set these to your desired credentials. */
const char *ssid = "AccessPointName";
const char *password = "SSID";

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
	server.send(200, "text/html", "<center><h1>Web Page Header 1!</h1></center> <ul><li><a href=""/temp.html"">Room Ambient Temperature</a></li></ul></center>");
}


/* --------------------------Tempature Web page --------------------------*/
void tempatureRoot() {
  tempread();
  digitalWrite(BUILTIN_LED, LOW);
  delay(5000);
  digitalWrite(BUILTIN_LED, HIGH);
}

void tempread() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16;
  fahrenheit = celsius * 1.8 + 32.0;
  char temp[10];
  String tempAsString;
  dtostrf(fahrenheit,1,2,temp); //dtostrf(value, width, precision, output);
  tempAsString = String(temp);

  server.send(200, "text/html", "<center>Ambient Air Temp:<br>" + tempAsString + " F</center>");
  Serial.print("  Temperature = ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
}
/* --------------------------Tempature Web page END --------------------------*/

void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Configuring access point...");
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
  server.on("/temp.html", tempatureRoot);
	server.begin();
	Serial.println("HTTP server started");
  /* On DEV board Blue led Setup */
  Serial.println("On DEV board Blue led Setup");
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.println("Total Setup Complete");
}

void loop() {
	server.handleClient();
}
