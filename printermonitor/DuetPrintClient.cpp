/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Additional Contributions:
/* 15 Jan 2019 : Owen Carter : Add psucontrol query via POST api call */

#include "DuetPrintClient.h"

DuetPrintClient::DuetPrintClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  updateDuetPrintClient(ApiKey, server, port, user, pass, psu);
}

void DuetPrintClient::updateDuetPrintClient(String ApiKey, String server, int port, String user, String pass, boolean psu) {
  server.toCharArray(myServer, 100);
  myApiKey = ApiKey;
  myPort = port;
  encodedAuth = "";
  if (user != "") {
    String userpass = user + ":" + pass;
    base64 b64;
    encodedAuth = b64.encode(userpass, true);
  }
  pollPsu = psu;
}

boolean DuetPrintClient::validate() {
  boolean rtnValue = false;
  printerData.error = "";
  if (String(myServer) == "") {
    printerData.error += "Server address is required; ";
  }
  if (myApiKey == "") {
    printerData.error += "ApiKey is required; ";
  }
  if (printerData.error == "") {
    rtnValue = true;
  }
  return rtnValue;
}

WiFiClient DuetPrintClient::getSubmitRequest(String apiGetData) {
  WiFiClient printClient;
  printClient.setTimeout(5000);
      if (printClient.connect("tornado.local", 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      printClient.println("GET /rr_status?type=3 HTTP/1.1");
      printClient.println();
    }


  Serial.println("Getting Duet Data via GET");
  Serial.println(apiGetData);
  result = "";
  if (printClient.connect(myServer, myPort)) {  //starts client connection, checks for connection
    printClient.print(String("GET /rr_status?type=3") + " HTTP/1.1\r\n" +
             "Host: " + myServer + "\r\n" +
             "Connection: close\r\n" +
             "\r\n"
            );
//            while (printClient.connected() || printClient.available())
//{
//  if (printClient.available())
//  {
//    String line = printClient.readStringUntil('\n');
//    Serial.println(line);
//  }
//}

//  apiGetData = "GET rr_status&type=3 HTTP/1.1";
//    printClient.println(apiGetData);
//    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
//    printClient.println("X-Api-Key: " + myApiKey);
//    if (encodedAuth != "") {
//      printClient.print("Authorization: ");
//      printClient.println("Basic " + encodedAuth);
//    }
//    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Connection: close");
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  } 
  else {
    Serial.println("Connection to Duet failed: " + String(myServer) + ":" + String(myPort)); //error message if no client connect
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Duet failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  // Check HTTP status
  char status[32] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.state = "";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!printClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    printerData.error = "Invalid response from " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
  }

  return printClient;
}

WiFiClient DuetPrintClient::getPostRequest(String apiPostData, String apiPostBody) {
  WiFiClient printClient;
  printClient.setTimeout(5000);

  Serial.println("Getting Octoprint Data via POST");
  Serial.println(apiPostData + " | " + apiPostBody);
  result = "";
  if (printClient.connect(myServer, myPort)) {  //starts client connection, checks for connection
    printClient.println(apiPostData);
    printClient.println("Host: " + String(myServer) + ":" + String(myPort));
    printClient.println("Connection: close");
    printClient.println("X-Api-Key: " + myApiKey);
    if (encodedAuth != "") {
      printClient.print("Authorization: ");
      printClient.println("Basic " + encodedAuth);
    }
    printClient.println("User-Agent: ArduinoWiFi/1.1");
    printClient.println("Content-Type: application/json");
    printClient.print("Content-Length: ");
    printClient.println(apiPostBody.length());
    printClient.println();
    printClient.println(apiPostBody);
    if (printClient.println() == 0) {
      Serial.println("Connection to " + String(myServer) + ":" + String(myPort) + " failed.");
      Serial.println();
      resetPrintData();
      printerData.error = "Connection to " + String(myServer) + ":" + String(myPort) + " failed.";
      return printClient;
    }
  } 
  else {
    Serial.println("Connection to Duet failed: " + String(myServer) + ":" + String(myPort)); //error message if no client connect
    Serial.println();
    resetPrintData();
    printerData.error = "Connection to Duet failed: " + String(myServer) + ":" + String(myPort);
    return printClient;
  }

  // Check HTTP status
  char status[32] = {0};
  printClient.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.1 409 CONFLICT") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    printerData.state = "";
    printerData.error = "Response: " + String(status);
    return printClient;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!printClient.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    printerData.error = "Invalid response from " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
  }

  return printClient;
}

void DuetPrintClient::getPrinterJobResults() {
  if (!validate()) {
    return;
  }
  //**** get the Printer Job status
  String apiGetData = "GET rr_status?type=3 HTTP/1.1";
  WiFiClient printClient = getSubmitRequest(apiGetData);
  if (printerData.error != "") {
    return;
  }
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + 710;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(printClient);
  if (!root.success()) {
    Serial.println("Duet Data Parsing failed: " + String(myServer) + ":" + String(myPort));
    printerData.error = "Duet Data Parsing failed: " + String(myServer) + ":" + String(myPort);
    printerData.state = "";
    return;
  }

//    printerData.toolTemp = "";
//    printerData.toolTargetTemp = "";
//    printerData.bedTemp = "";
//    printerData.bedTargetTemp = (const char*)root["temps"]["bed"]["current"];
//    printerData.toolTemp = (const char*)root["temps"]["tools"]["active"];
//  printerData.toolTargetTemp = (const char*)root2["temperature"]["tool0"]["target"];
  printerData.bedTemp = (const char*)root["temps"]["tools"]["active"];
//  printerData.bedTargetTemp = (const char*)root2["temperature"]["bed"]["target"];
    
//  printerData.averagePrintTime = (const char*)root["job"]["averagePrintTime"];
  printerData.estimatedPrintTime = (const char*)root["printDuration"];
//  printerData.fileName = (const char*)root["job"]["file"]["name"];
//  printerData.fileSize = (const char*)root["job"]["file"]["size"];
//  printerData.lastPrintTime = (const char*)root["job"]["lastPrintTime"];
  printerData.progressCompletion = (const char*)root["fractionPrinted"];
//  printerData.progressFilepos = (const char*)root["progress"]["filepos"];
  printerData.progressPrintTime = (const char*)root["time"];
  printerData.progressPrintTimeLeft = (const char*)root["timesLeft"]["file"];
//  printerData.filamentLength = (const char*)root["job"]["filament"]["tool0"]["length"];
//  printerData.state = (const char*)root["status"];
//  printerData.state = "Operational";
  //CB needs proper detection
  //{"status":"P"
  
  if (isOperational()) {
    Serial.println("Status: " + printerData.state);
  } else {
    Serial.println("Printer Not Operational");
  }
  //**** get the Printer Temps and Stat
  apiGetData = "GET /rr_status&type=3 HTTP/1.1";
  printClient = getSubmitRequest(apiGetData);
  if (printerData.error != "") {
    return;
  }
  const size_t bufferSize2 = 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(9) + 300;
  DynamicJsonBuffer jsonBuffer2(bufferSize2);

  // Parse JSON object
//  JsonObject& root2 = jsonBuffer2.parseObject(printClient);
//  if (!root2.success()) {
//    printerData.isPrinting = false;
//    printerData.toolTemp = "";
//    printerData.toolTargetTemp = "";
//    printerData.bedTemp = "";
//    printerData.bedTargetTemp = (const char*)root["temps"]["bed"]["current"];
//    return;
//  }

  String printing = (const char*)root["status"];
  if (printing == "P") {
    printerData.isPrinting = true;
  }
  printerData.toolTemp = (const char*)root["temps"]["tools"]["active"][0][0];
//  Serial.println("toolTemp: " + printerData.toolTemp );
//  printerData.toolTemp = "123";
//  printerData.toolTargetTemp = (const char*)root2["temperature"]["tool0"]["target"];
  printerData.bedTemp = (const char*)root["temps"]["bed"]["current"];
//  printerData.bedTargetTemp = (const char*)root2["temperature"]["bed"]["target"];

  if (isPrinting()) {
    Serial.println("Status: " + printerData.state + " " + printerData.fileName + "(" + printerData.progressCompletion + "%)");
  }
}

void DuetPrintClient::getPrinterPsuState() {
  //**** get the PSU state (if enabled and printer operational)
  if (pollPsu && isOperational()) {
    if (!validate()) {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on.
      return;
    }
    String apiPostData = "POST /api/plugin/psucontrol HTTP/1.1";
    String apiPostBody = "{\"command\":\"getPSUState\"}";
    WiFiClient printClient = getPostRequest(apiPostData,apiPostBody);
    if (printerData.error != "") {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on.
      return;
    }
    const size_t bufferSize3 = JSON_OBJECT_SIZE(2) + 300;
    DynamicJsonBuffer jsonBuffer3(bufferSize3);
  
    // Parse JSON object
    JsonObject& root3 = jsonBuffer3.parseObject(printClient);
    if (!root3.success()) {
      printerData.isPSUoff = false; // we do not know PSU state, so assume on
      return;
    }
  
    String psu = (const char*)root3["isPSUOn"];
    if (psu == "true") {
      printerData.isPSUoff = false; // PSU checked and is on
    } else {
      printerData.isPSUoff = true; // PSU checked and is off, set flag
    }
    printClient.stop(); //stop client
  } else {
    printerData.isPSUoff = false; // we are not checking PSU state, so assume on
  }
}

// Reset all PrinterData
void DuetPrintClient::resetPrintData() {
  printerData.averagePrintTime = "";
  printerData.estimatedPrintTime = "";
  printerData.fileName = "";
  printerData.fileSize = "";
  printerData.lastPrintTime = "";
  printerData.progressCompletion = "";
  printerData.progressFilepos = "";
  printerData.progressPrintTime = "";
  printerData.progressPrintTimeLeft = "";
  printerData.state = "";
  printerData.toolTemp = "";
  printerData.toolTargetTemp = "";
  printerData.filamentLength = "";
  printerData.bedTemp = "";
  printerData.bedTargetTemp = "";
  printerData.isPrinting = false;
  printerData.isPSUoff = false;
  printerData.error = "";
}

String DuetPrintClient::getAveragePrintTime(){
  return printerData.averagePrintTime;
}

String DuetPrintClient::getEstimatedPrintTime() {
  return printerData.estimatedPrintTime;
}

String DuetPrintClient::getFileName() {
  return printerData.fileName;
}

String DuetPrintClient::getFileSize() {
  return printerData.fileSize;
}

String DuetPrintClient::getLastPrintTime(){
  return printerData.lastPrintTime;
}

String DuetPrintClient::getProgressCompletion() {
  return String(printerData.progressCompletion.toInt());
}

String DuetPrintClient::getProgressFilepos() {
  return printerData.progressFilepos;  
}

String DuetPrintClient::getProgressPrintTime() {
  return printerData.progressPrintTime;
}

String DuetPrintClient::getProgressPrintTimeLeft() {
  String rtnValue = printerData.progressPrintTimeLeft;
  if (getProgressCompletion() == "100") {
    rtnValue = "0"; // Print is done so this should be 0 this is a fix for OctoPrint
  }
  return rtnValue;
}

String DuetPrintClient::getState() {
  return printerData.state;
}

boolean DuetPrintClient::isPrinting() {
  return printerData.isPrinting;
}

boolean DuetPrintClient::isPSUoff() {
  return printerData.isPSUoff;
}

boolean DuetPrintClient::isOperational() {
  boolean operational = false;
  if (printerData.state == "P" || isPrinting()) {
    operational = true;
  }
  return operational;
}

String DuetPrintClient::getTempBedActual() {
  return printerData.bedTemp;
}

String DuetPrintClient::getTempBedTarget() {
  return printerData.bedTargetTemp;
}

String DuetPrintClient::getTempToolActual() {
  return printerData.toolTemp;
}

String DuetPrintClient::getTempToolTarget() {
  return printerData.toolTargetTemp;
}

String DuetPrintClient::getFilamentLength() {
  return printerData.filamentLength;
}

String DuetPrintClient::getError() {
  return printerData.error;
}

String DuetPrintClient::getValueRounded(String value) {
  float f = value.toFloat();
  int rounded = (int)(f+0.5f);
  return String(rounded);
}
