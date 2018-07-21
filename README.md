# energydisplay-m5

![Finished display](https://i.imgur.com/7l4xqym.png)

![Finished display](https://i.imgur.com/N01PsK1.gifv)

This is an energy status display that uses the M5Stack module, which features a 320x240 color LCD and an ESP32 with Wifi support.
The data is fetched from a Raspberry Pi or Beaglebone or any http web server for that matter. Getting the data into the relevant files is done elsewhere, this is just a display. There's one file for the core values, and then two text files for the histogram data for PV energy production and domestic power usage.

The center button on the M5Stack is used to toggle power, in case you have a battery connected. The left and right buttons switch through various display modes. Data is refreshed every 60 seconds and the current screen is automatically updated. No extra libraries have been used.

A black/white e-ink version without animations can be found on my github as well.

