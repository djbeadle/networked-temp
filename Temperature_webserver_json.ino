/*
 * 
 * A network connected temperature sensor!
 * Uses an Arduino Ethernet Shield and a QTechKnow temperature sensor
 * 
 * Responds to GET requests with the temperature data in JSON format.
 * POSTs the current readings to an InfluxDB server once a minute.
 * 
 * All around good stuff.
 * 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <RBD_Timer.h> // https://github.com/alextaujenis/RBD_Timer

RBD::Timer timer;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 151);
const int ArduSensorPin = 0;  // the pin that our ArduSensor is connected to

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Info of the server we are connecting to every three minutes:
IPAddress my_server(192,168,1,152);
int my_server_port = 8086;

// Initialize the Ethernet client library
EthernetClient g_client;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  timer.setTimeout(60000); // 60,000 once a minute
  timer.restart();
}


void loop() {
  if(timer.onRestart())
  {
    update_server(g_client, my_server, my_server_port);
  }
  
  // listen for incoming clients and return the current temperature
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          // output the value of each analog input pin
          /*
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          */
          read_temperature(client);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void read_temperature(EthernetClient client)
{
  int value = analogRead(ArduSensorPin);   // read the ArduSensor Temperature
  float millivolts = (value / 1024.0) * 5000;  // crunch some numbers
  float fahrenheit = millivolts / 10;  // sensor output is 10mV per degree F
  Serial.print(fahrenheit);            // print the Fahrenheit
  Serial.print(" degrees Fahrenheit, ");

  Serial.print( (fahrenheit - 32) * 5 / 9 );  //  convert to Celsius
  Serial.println(" degrees Celsius");

  /*
  client.print(fahrenheit);            // print the Fahrenheit
  client.print(" degrees Fahrenheit, ");

  client.print( (fahrenheit - 32) * 5 / 9 );  //  convert to Celsius
  client.println(" degrees Celsius");
  */

  client.print("{\"fahrenheit\":");
  client.print(fahrenheit);
  client.print(",");
  client.print("\"celsius\":");
  
  client.print((fahrenheit - 32) * 5 / 9 );
  client.print("}");
}

void update_server(EthernetClient client, IPAddress server, int port)
{
  Serial.println("Trying to update the server.");
  int value = analogRead(ArduSensorPin);   // read the ArduSensor Temperature
  float millivolts = (value / 1024.0) * 5000;  // crunch some numbers
  float fahrenheit = millivolts / 10;  // sensor output is 10mV per degree F

  String payload = "daniel_rm_faren,host=arduino01,region=us-east value=";
  payload.concat(fahrenheit);
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("Payload length: ");
  Serial.println(payload.length());
  
  if(client.connect(my_server, port)){
    client.println("POST /write?db=FiveOhTwo HTTP/1.1");
    client.print("Host: ");
    client.println(my_server);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("content-length: ");
    client.println(payload.length());
    client.println("");
    client.println(payload);
    Serial.println("1");
  }
  else {
    Serial.println("connection failed");
    Serial.println();
  }

  Serial.println("Response from the server:");
  while(client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available()) { //connected or data available
    char c = client.read();
    Serial.print(c);
    if(isControl(c))
    {
      break;
    }
  }

  Serial.println();
  Serial.println("disconnecting.");
  Serial.println("==================");
  Serial.println();
  client.stop();
}

