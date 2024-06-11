# GlucoseLED
ESP 8266 project to change LEDs colors depending on glucose level retrieved using Dexcom share service.

The code to connect to Dexcom Share service is based on [pydexcom](https://github.com/gagebenne/pydexcom).

As explained in the page above, the prerequisite for this to work is to download the Dexcom G6 / G5 / G4 mobile app and enable the Share service.
The Dexcom Share service requires setup of at least one follower to enable the share service, but glucodeLED will use your credentials, not the follower's.

This project uses the [WiFiManager](https://github.com/tzapu/WiFiManager) library so that the wifi and Dexcom credentials can be easily set up.

## Technical details
By default the number of LED is 7, data pin is D7 and LED type is WS2812B. The URL of the Dexcom Share service depends on whether your Dexcom account is US based or not. The default URL is for non US based account.

All these settings along with the glucose level ranges and colors can be easily changed in the code.

The code was tested on a Wemos D1 mini but it will most likely work on other boards.
