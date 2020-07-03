//EspAsyncExampleUpdateInoFromBrowser.ino
// taken from https://github.com/me-no-dev/ESPAsyncWebServer#setting-up-the-server  20200702
//#include "ESPAsyncTCP.h" // for 8266
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h> // fix error: 'SPIFFS' was not declared in this scope
#include <Update.h> // required to fix 'Update' was not declared in this scope

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

const char* ssid = "your-ssid";
const char* password = "your-pass";
const char* http_username = "admin";
const char* http_password = "admin";

//flag to use from web update to reboot the ESP
bool shouldReboot = false;

void onRequest(AsyncWebServerRequest *request) {
  Serial.printf("onRequest 404:%s\n", request->url().c_str());
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.printf("onBody:%s\n", request->url().c_str());
  //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  Serial.printf("onUpload:%s\n", request->url().c_str());
  //Handle upload
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  //Handle WebSocket event

}

void setup() {
  Serial.begin(115200);
  Serial.printf("it begins\n");
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    Serial.printf("maybe SSID(%s) or password(%s) needs adjusting\n", ssid, password);
    return;
  }
  Serial.printf("Connected\n");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.printf("get request:%s\n", request->url().c_str());
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    Serial.printf("post request:%s\n", request->url().c_str());
    request->send(200);
  }, onUpload);

  // send a file when /index is requested
  server.on("/index", HTTP_ANY, [](AsyncWebServerRequest * request) {
    String targetFile = request->url();
    targetFile += ".html";
    Serial.printf(" request:%s\n", request->url().c_str());
    if (SPIFFS.exists(targetFile))
    {
      request->send(SPIFFS, targetFile);
    }
    else
    {
      Serial.printf("spiffs says file:%s, does not exist\n", targetFile.c_str());
    }
  });

  server.on("/", HTTP_ANY, [](AsyncWebServerRequest * request) {
    String targetFile = request->url();
    targetFile = "/index.html";
    Serial.printf(" request:%s\n", request->url().c_str());
    if (SPIFFS.exists(targetFile))
    {
      request->send(SPIFFS, targetFile);
    }
    else
    {
      Serial.printf("spiffs says file:%s, does not exist\n", targetFile.c_str());
    }
  });

  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.printf("get request:%s\n", request->url().c_str());
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    else
      Serial.printf("request->authenticate returned true, close browser to try again\n");
    request->send(200, "text/plain", "Login Success!");
  });

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.printf("get request:%s\n", request->url().c_str());
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    Serial.printf("post request:%s\n", request->url().c_str());
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      //Update.runAsync(true); // error: 'class UpdateClass' has no member named 'runAsync'
      // https://gitter.im/espressif/arduino-esp32?at=5cc1efb7a4ef097471face9c
      // Me No Dev @me-no-dev Apr 26 2019 05:05 remove the call it's not needed in ESP32
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // attach filesystem root at URL /fs
  server.serveStatic("/fs", SPIFFS, "/");

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.begin();
}

void loop() {
  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  static char temp[128];
  sprintf(temp, "Seconds since boot: %lu", millis() / 1000); // u to lu, format '%u' expects argument of type 'unsigned int', but argument 3 has type 'long unsigned int' [-Werror=format=]

  events.send(temp, "time"); //send event "time"
}
