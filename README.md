# EspAsync-Example-Update-Ino-From-Browser
From https://github.com/me-no-dev/ESPAsyncWebServer#setting-up-the-server

I am using the Esp32.
As I was learning to use ESPAsyncWebServer I copied the code from the setting up the server section.

It threw errors:

'SPIFFS' was not declared in this scope.

'Update' was not declared in this scope.

'class UpdateClass' has no member named 'runAsync' .

The first two were easy.

add #include <SPIFFS.h> .

add #include <Update.h>

The last took some more digging.

//Update.runAsync(true); // error: 'class UpdateClass' has no member named 'runAsync'

// https://gitter.im/espressif/arduino-esp32?at=5cc1efb7a4ef097471face9c

// Me No Dev @me-no-dev Apr 26 2019 05:05 remove the call it's not needed in ESP32


The result is this ino that compiles and works.

Do change the SSID and password for your system.


Many thanks to Me No Dev for his wonderful code.

Edit: change to no wrap

