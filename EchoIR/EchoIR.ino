/*
 Name:		EchoIR.ino
 Created:	12/26/2016 11:06:14 PM
 Author:	luca
*/

#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

fauxmoESP fauxmo;
IRrecv irrecv(2);
decode_results results;
IRsend irsend(0);
ESP8266WebServer server(80);
String webhead = "<html><head><title>IR Hub</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style type=\"text/css\">form { display: inline; }</style></head><body>";
String webfoot = "</body></html>";

bool savenext = false;
String savenextname;

#include "pw.h"
//#define WIFI_SSID "..."
//#define WIFI_PASS "....."
#define SAVE_MAX 10
#define SAVE_SIZE 100
#define SAVE_I_USED 0
#define SAVE_I_LENGTH 1
#define SAVE_NAME 2
#define SAVE_DATA 15
void setup() {
	Serial.begin(115200);
	wifiSetup();
	webServerSetup();
	OTASetup();
	EEPROM.begin(1024);
	irsend.begin();

	for (int i = 0; i < SAVE_MAX; i++) {
		if (EEPROM.read(i*SAVE_SIZE + SAVE_I_USED) == 1) {
			String name = "";
			for (int ic = 0; ic < 13; ic++) {
				int c = EEPROM.read(i*SAVE_SIZE + SAVE_NAME + ic);
				if (c == 255) break;
				name += (char)c;
			}
			if (name.length() == 0) continue;
			char namea[name.length()];
			name.toCharArray(namea, name.length()+1);
			Serial.println(namea);
			fauxmo.addDevice(namea);
		}
		fauxmo.onMessage(callback);
	}
}
void callback(const char * device_name, bool state) {
	Serial.print("Echo: ");
	Serial.println(device_name);
	for (int i = 0; i < SAVE_MAX; i++) {
		if (EEPROM.read(i*SAVE_SIZE + SAVE_I_USED) == 1) {
			bool correct = true;
			for (int ic = 0; ic < 13; ic++) {
				int c = EEPROM.read(i*SAVE_SIZE + SAVE_NAME + ic);
				if (c == 255) {
					if (device_name[ic] != '\0')
						correct = false;
					break;
				}
					
				if ((char)c != device_name[ic]) correct = false;
			}
			if (correct) {
				Serial.println("Found");
				play(i);
				break;
			}
		}
	}
}

void wifiSetup() {

	// Set WIFI module to STA mode
	WiFi.mode(WIFI_STA);

	// Connect
	Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	// Wait
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();

	// Connected!
	Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}
void webServerSetup() {
	server.on("/", webRoot);
	server.on("/save", webSave);
	server.on("/play", webPlay);
	server.on("/del", webDel);
	server.on("/restart", webRestart);
	server.onNotFound([]() {
		server.send(404, "text/plain", "not found");
	});

	//here the list of headers to be recorded
	const char * headerkeys[] = { "User-Agent","Cookie" };
	size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
	//ask server to track these headers
	server.collectHeaders(headerkeys, headerkeyssize);
	server.begin();
}
void OTASetup() {
	ArduinoOTA.onStart([]() {
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
}
void redirect(String url) {
	server.sendHeader("Location", url);
	server.sendHeader("Cache-Control", "no-cache");
	server.send(301);
}
void webRestart() {
	redirect("/");
	ESP.restart();
}
void webRoot() {
	String content = "";

	for (int i = 0; i < SAVE_MAX; i++) {
		content += "<span>IR " + String(i + 1) + ": ";
		if (EEPROM.read(i*SAVE_SIZE + SAVE_I_USED) != 1) content += "EMPTY";
		else {
			String name = "";
			for (int ic = 0; ic < 13; ic++) {
				int c = EEPROM.read(i*SAVE_SIZE + SAVE_NAME + ic);
				if (c == 255) break;
				name += (char)c;
			}
			content += name;
			content += " <a href='/del?id=" + String(i) + "' >Delete</a>";
			content += " <a href='/play?id=" + String(i) + "' >Test</a>";
		}
		content += "</span><br />";
	}

	content += "<h3>Add:</h3>";
	if (savenext) content += "<span>Wait for IR</span><br />";
	content += "<form action='/save' method='POST'><br>";
	content += "Name: <input type='text' name='name'><br>";
	content += "<input type='submit' name='SUBMIT' value='Add'></form>";

	content += " <a href='/restart' >Restart(Apply)</a>";
	server.send(200, "text/html", webhead + content + webfoot);
}
void webSave() {
	if (server.arg("name").length() >= 1) {
		savenext = true;
		savenextname = server.arg("name");
		irrecv.enableIRIn(); 
	}
	redirect("/");
}

void webDel() {
	int id = server.arg("id").toInt();
	EEPROM.write(id*SAVE_SIZE + SAVE_I_USED, 255);
	EEPROM.commit();
	redirect("/");
}

void webPlay() {
	Serial.println("Play");
	int id = server.arg("id").toInt();
	
	play(id);
	
	Serial.println("Done");
	redirect("/");
}

void loop() {
	if (savenext) { savenext = !save(); }
	server.handleClient();
	ArduinoOTA.handle();
}


void play(int id) {
	if (id >= SAVE_MAX) return;
	int len = EEPROM.read(id*SAVE_SIZE + SAVE_I_LENGTH);
	unsigned int raw[len];
	Serial.println(len);
	for (int i = 0; i < len; i++) {
		raw[i] = EEPROM.read(id*SAVE_SIZE + SAVE_DATA + i)*USECPERTICK;
		Serial.println(raw[i]);
	}
	irsend.sendRaw(raw, len, 38);
}


bool save() {
	if (irrecv.decode(&results)) {
		int id = 0;
		while (EEPROM.read(id*SAVE_SIZE + SAVE_I_USED) == 1) { id++; }
		if (id >= SAVE_MAX) {
			void disableIRIn();
			return true;
		}
		Serial.println("Found");
		int count = results.rawlen;
		Serial.println(count);
		EEPROM.write(id*SAVE_SIZE + SAVE_I_USED, 1);
		EEPROM.write(id*SAVE_SIZE + SAVE_I_LENGTH, count - 1);


		for (int i = 0; i<savenextname.length(); i++) {
			EEPROM.write(id*SAVE_SIZE + SAVE_NAME + i, savenextname.charAt(i));
		}
		EEPROM.write(id*SAVE_SIZE + SAVE_NAME + savenextname.length(), 255);
		unsigned int raw[count - 1];
		for (int i = 1; i < count; i++) {
			raw[i - 1] = results.rawbuf[i] * USECPERTICK;
			EEPROM.write(id*SAVE_SIZE + SAVE_DATA + i - 1, results.rawbuf[i]);
			Serial.println(results.rawbuf[i]);
		}
		EEPROM.commit();
		irrecv.resume();
		Serial.println("Done");
		void disableIRIn();
		return true;
	}
	return false;
}
