# EchoIR
Controll IR devices with Alexa (Amazon Echo) using ESP8266   

This Sketch emulates a Belkin WeMo device [fauxmoesp](https://bitbucket.org/xoseperez/fauxmoesp) 
and sends Raw IR signals [IRremoteESP8266](https://github.com/markszabo/IRremoteESP8266) to your IR devices

## Installation
- Add ESP8266 support to your ArduinoIDE (https://github.com/esp8266/Arduino)
- Install the dependencies listed below
- Upload the [Sketch](https://github.com/Lucaber/EchoIR/blob/master/EchoIR/EchoIR.ino) to your ESP8266 board
- Connect an IR-Led to Pin 0 and the data pin of an IR photo sensor to Pin 2
- Open the Webpage of your ESP8266 and add your devices

## Dependencies
- FauxmoESP (https://bitbucket.org/xoseperez/fauxmoesp)
- IRremoteESP8266 (https://github.com/markszabo/IRremoteESP8266)
