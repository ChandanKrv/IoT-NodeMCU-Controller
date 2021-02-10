/*
By Chandan Kumar
https://github.com/chandanKrv
Node MCU Controller Using Web or App
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);//Establishing Local server at port 80 whenever required

String serverNameWithAuth, st, content, outputsState,esid,epass,authUser,boardNo;
// Your IP address or domain name with URL path
String serverName = "http://node.apkof.com/esp-outputs-action.php?action=os&auth=";

// Update interval time set to 5 seconds
const long interval = 3000;
unsigned long previousMillis = 0;

//const char* ssid = "text";

int i = 0, statusCode;

//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);

void setup()
{
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  //----------------------------------------START Read eeprom for ssid, pass and authEmail------------
  Serial.println("Reading EEPROM SSID");
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);


  Serial.println("Reading EEPROM Password");
  for (int i = 32; i < 64; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  Serial.println("Reading EEPROM Auth: ");
  for (int i = 64; i < 75; ++i)
  {
    authUser += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("Auth ID: ");
  Serial.println(authUser);
  
  Serial.println("Reading EEPROM Board No.");
  for (int i = 75; i < 76; ++i)
  {
    boardNo += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("Board No.: ");
  Serial.println(boardNo);

  //--------------------------------------END Read eeprom for ssid, pass and authEmail------------

  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    Serial.print("Auth User ID: ");
    Serial.println(authUser);  
   
    serverNameWithAuth = serverName + authUser;  
       
    Serial.println(serverNameWithAuth);
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");

  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(500);
    server.handleClient();
  }

}
void loop() {
  

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // Check WiFi connection status
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      outputsState = httpGETRequest(serverNameWithAuth);
      Serial.println(outputsState);
      JSONVar myObject = JSON.parse(outputsState);

      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.println(serverNameWithAuth);

      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();

      for (int i = 0; i < keys.length(); i++) {
        JSONVar value = myObject[keys[i]];
        Serial.print("GPIO: ");
        Serial.print(keys[i]);
        Serial.print(" - SET to: ");
        Serial.println(value);
        pinMode(atoi(keys[i]), OUTPUT);
        digitalWrite(atoi(keys[i]), atoi(value));
        //        Serial.print("Auth: ");
        //        Serial.println(authEmailId);
        //        Serial.print("Server: ");
        Serial.println(serverNameWithAuth);
        Serial.println(" ");
      }
      // save the last HTTP GET Request
      previousMillis = currentMillis;
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}


String httpGETRequest(String serverNameWithAuth) {
  WiFiClient client;
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(client, serverNameWithAuth);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(serverNameWithAuth);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}


//----------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(1000);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("NodeMCU", "");//Hotspot name,pass
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{
  {
    server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE html> \n"
                "<html> \n"
                "<style> \n"
                "\tform { \n"
                "\t\tborder: 3px solid #b8afaf; \n"
                "\t} \t\n"
                "\tinput[type=text], \n"
                "\tinput[type=number],\n"
                "    input[type=email] { \n"
                "\t\twidth: 100%; \n"
                "\t\tpadding: 12px 20px; \n"
                "\t\tmargin: 8px 0; \n"
                "\t\tdisplay: inline-block; \n"
                "\t\tborder: 1px solid #ccc; \n"
                "\t\tbox-sizing: border-box; \n"
                "\t} \n"
                "\t\t\n"
                "\tbutton { \n"
                "\t\tbackground-color: #259870; \n"
                "\t\tcolor: white; \n"
                "\t\tpadding: 14px 20px; \n"
                "\t\tmargin: 8px 0; \n"
                "\t\tborder: none; \n"
                "\t\tcursor: pointer; \n"
                "\t\twidth: 100%; \n"
                "\t} \n"
                "\t\t\n"
                "\tbutton:hover { \n"
                "\t\topacity: 0.8; \n"
                "\t} \n"
                "\t\n"
                "\t.container { \n"
                "\t\tpadding: 16px; \n"
                "\t}\n"
                "\n"
                "    h2{\n"
                "        text-align: center;\n"
                "    } \t\n"
                "\n"
                "</style> \n"
                "<body> \n"
                "    <h2>ESP Board Setup</h2> \n"
                "    <p>Enter your WiFi credentials, Following data will be uploaded to ESP board.</p>\n"
                "    <form method = 'get' action = 'setting'>\n"
                "\t\t<div class=\"container\">\n"
                "            <label><b>Available WiFi</b></label> \n"
                "            <p>";
      content += st;
      content += " </p>\n"
                 "\t\t\t<label><b>SSID</b></label> \n"
                 "\t\t\t<input type=\"text\" placeholder=\"Enter Your SSID (WiFi Name)\" name=\"ssid\" length = 32 required> \n"
                 "\t\t\t<label><b>Password</b></label> \n"
                 "\t\t\t<input type=\"text\" placeholder=\"Enter WiFi Password\" name=\"pass\" length = 32 required> \n"
                 "\t\t\t<label><b>Mobile Number </b> [ Must use same mobile number on our website ]</label> \n"
                 "            <input type=\"number\" placeholder=\"Enter 10 digits mobile number\" name=\"authPh\" oninput=\"javascript: if (this.value.length > this.maxLength) this.value = this.value.slice(0, this.maxLength);\" maxlength = \"10\" required> \n"
                 "\t\t\t<label><b>Board Number </b> [ If this is your first ESP board then enter 1 ]</label> \n"
                 "\t\t\t<input type=\"number\" placeholder=\"Enter ESP Board Number\" name=\"board\" oninput=\"javascript: if (this.value.length > this.maxLength) this.value = this.value.slice(0, this.maxLength);\" maxlength = \"1\" required>           \t\t\n"
                 "\t\t\t<button type=\"submit\">Submit Now</button> \n"
                 "\t\t</div> \n"
                 "\t</form> \n"
                 "</body> \n"
                 "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String onlyPh = server.arg("authPh");
      String qboard = server.arg("board");
      String qAuth = qboard + onlyPh;
      if (qsid.length() > 0 && qpass.length() > 0 && qAuth.length() > 0 && qboard.length() > 0) {
        Serial.println("Clearing eeprom");
        for (int i = 0; i < 106; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println(qAuth);
        Serial.println("");
        Serial.println(qboard);
        Serial.println("");

        Serial.println("Writing eeprom SSID: ");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }


        Serial.println("Writing eeprom Pass: ");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }


        Serial.println("Writing eeprom Auth ID: ");
        for (int i = 0; i < qAuth.length(); ++i)
        {
          EEPROM.write(64 + i, qAuth[i]);
          Serial.print("Wrote: ");
          Serial.println(qAuth[i]);
        }

        Serial.println("Writing eeprom Board No.: ");
        for (int i = 0; i < qboard.length(); ++i)
        {
          EEPROM.write(75 + i, qboard[i]);
          Serial.print("Wrote: ");
          Serial.println(qboard[i]);
        }

        EEPROM.commit();

        content = " {\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  }
}
