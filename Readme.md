# Whater Level Monitoring

With this project I want to monitor and alert me via telegram when the level of water in the condensation tank is too high. I live in appartament and I can't drain the condensation water anywhere other than into a container. When the tank is full the water go down to my neighbor and I don't want to fight for this :) 


# Hardware

I used a low cost WROOM 02 board based on ESP8266 with an on-board battery management system and battery holder for 18650 Li-ion battery [ArduinoForum](https://forum.arduino.cc/t/esp-wroom-02-with-battery-socket-does-it-need-extra-resistors-to-monitor-4-2v/483862)

To read the battery level I connected a 100k resistor from battery + pole to A0 pin, all the remaining stuff was did in firmware

To measure the tank levele I used an economic but effective ultrasound meter [HC-SR04](https://amzn.eu/d/0c41vQpR)

And finally the hose adapter to drain water into tank is a 3D object created to adapt the space available, more info at [ThingIverse](https://www.thingiverse.com/thing:7382079)

I had to solder RESET to GPIO16 to enable deep standby so I can use a battery for my circuit, WiFi is an enemy for batteries.

I created a 3d model for hose adapter for my condensation drainage into the tank, this allow the existances of hose and ultrasound meter [Condensa Level Monitor](./3dmodel/Condensa%20Level%20Monitor.png)

# Cloud to see data everywhere

I used ThingSpeak because is very simple ad I used it in the past for other small projects, my device is visible at [AllertaCondensa](https://thingspeak.mathworks.com/channels/3425868)

# Firmware

No more to say about it, simply read data about distance and find an average value, send it to ThingSpeak and evaluate if the level exceed the minimum distance for warning or for alert.
If in warning region (level between warning level and alert level) send once a message to Telegram chat_id you specify, if in alert region (below alert level) send every minute a message to Telegram

# Assembly

One picture say more than words

![Mounted](./3dmodel/Condensa%20Level%20Monitor%20mounted.jpeg)