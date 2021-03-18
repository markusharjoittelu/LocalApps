/*********
  Arduino project for reading humidity and temperature from DHT22 sensor
  and time using Network Time Protocol (NTP) and displaying data in 
  webserver which runs in LoLin NodeMcu V3 with esp8266 microchip
*********/

// Here we import necessary libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Variables to save network credentials
// Replace these with available network credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Variable for time zone UTC offset
const long utcOffsetInSeconds = 7200;

//Variables to hold date and time in strings
String allTheTime = "";

//Variable for data string
String data = "";

//Function for joining time string
void handleTime(NTPClient);

// Create WiFiUDP object
WiFiUDP ntpUDP;
// Define NTP Client to get time
// Use NTP Server pool.ntp.org which is open NTP project
NTPClient timeClient(ntpUDP, utcOffsetInSeconds);

#define DHTPIN 5     // We use GPIO5 (D1) pin of the chip for sensor data pin

// Select which sensor you are using by uncommenting correct one
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

//Create sensor object
DHT dht(DHTPIN, DHTTYPE);

// Variables for temperature and humidity
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Variable for storing last update time of the sensor
unsigned long previousMillis = 0;    

// Variable for sensor update interval time in milliseconds
const long interval = 10000;  

//Array for webserver code
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" charset='utf-8' content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Pottukellarin lämpötila ja suhteellinen kosteus</h2>
  <p>
    <i class="fas fa-clock" style="color:#059e8a;"></i> 
    <span class="dht-labels"></span> 
    <span id="time">%TIME%</span>
  <p/>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Lämpötila: </span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Suhteellinen kosteus: </span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("time").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/time", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with sensor values
String processor(const String& var){
  if(var == "TIME"){
    return String(allTheTime);
  }
  else if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  //Start sensoring
  dht.begin();
  
  // Connect to network
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 server local IP address
  Serial.println(WiFi.localIP());
  // Initialize NTP client
  timeClient.begin();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(allTheTime).c_str());
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(data).c_str());
  });
  //Add header to avoid problems with javascript CORS check
  //Probably security risk
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); 
  //Start server
  server.begin();
}
 
void loop(){      
  //save current time
  unsigned long currentMillis = millis();
  //check if specified time has passed
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the sensor values
    previousMillis = currentMillis;
    // Update time from NTP server
    timeClient.update(); 
    //use handleTime function to modify time to more suitable form
    handleTime(timeClient);
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // check is temperature read value number, if not, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // check is humidity read value number, if not, don't change h value
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    //Serialize data to "JSON"-form
    data = "{\"time\":\"";
    data += allTheTime;
    data += "\",\"temperature\":\"";
    data += t;
    data += "\",\"humidity\":\"";
    data += h;
    data += "\"}";
    Serial.println(data);
    }
  }
}

//modify time to more suitable form
void handleTime(NTPClient tc) {
  //variables to hold data
  String formattedDate;
  String timeStamp;
  //get date from NTPClient
  formattedDate = tc.getFormattedDate();
  Serial.println(formattedDate);
  // Extract date
  int splitT = formattedDate.indexOf("T");
  String d, y, m;
  y = formattedDate.substring(0,4);
  m = formattedDate.substring(5,7);
  d = formattedDate.substring(8,10);  
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  //form suitable date and time string
  allTheTime = d + "." + m + "." + y + " " + timeStamp;
  }
