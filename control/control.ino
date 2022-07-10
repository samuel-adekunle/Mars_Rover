#include "include/connection.hpp"
#include "include/command_interpreter.hpp"
#include "include/vision_interpreter.hpp"
const bool REQUIRE_WIFI_CONNECTION = false;
const bool REQUIRE_COMMAND_CONNECTION = false;

void setup() {
	Serial.begin(115200);
	Serial2.begin(115200);
	Connections::setupAllConnections(REQUIRE_WIFI_CONNECTION, REQUIRE_COMMAND_CONNECTION);
}


void loop() {

	if(Serial.available()){
		String command = Serial.readStringUntil('\n');
		if(command == "reset"){
      ESP.restart();
		}
		else if(command == "sendStatus"){
			Serial.println(command);
			Subsystem_Status status;
			status.send();
			Serial.print("> ");
		}
		else if(command == "pong"){
			Pong p;
			p.send();
		}
		else if(command == "ball"){
			//0525252 00920070 01f10197
			// int a = 0x920070;
			// int b = 0x1f10197;
			// int a = 0xe8;
			// int b = 0x27f01ac;

			// int a = 0x0006017f;
			// int b = 0x005f01dd;

			// int a = 0x017e00ff; //right
			// int b = 0x024f0192;

//			int a= 	0xe600da;
//			int b = 0xa001dd;
//
//			unsigned int x1 = a >> 16;
//			unsigned int y1 = (a << 16 ) >> 16;
//
//			unsigned int x2 = b >> 16;
//			unsigned int y2 = (b << 16 ) >> 16;
//
//			unsigned int bounds[] = {x1,y1,x2,y2};
      		unsigned int bounds[] = {0xdd,0x48,0xea4c,0x1};

			Serial.println("Distance " + String(get_distance(bounds)) + " cm");
			Serial.println("Angle " + String(get_angle(bounds)) + " rad");
      		Serial.println(">");
		}
		else if (command=="testDrive"){
			Connections::stopOnSite = true;
			Serial.println(">");
		}
		else if (command=="p"){
			while(!Serial.available());
			//Serial2.println(Serial.readStringUntil('\n'));
			Serial2.println("f300,20,?\n");
			Serial.println("Done!");
		}
		else if (command=="a"){
			Serial2.println("a?\n");
			long start = millis();
			bool timeout = false;
			while(!Serial2.available()){
				if((millis() - start) > 5000){
					Serial.println("TIMEOUT");
					timeout = true;
					break;
				}
			}
			if(!timeout){
				Serial.println(Serial2.readStringUntil('?'));
			}
		}
		else if(command == "d"){
			Serial2.println("d?");
			Serial.println("d?");
			Serial.print(">");
		}
	}
	Connections::pollConnections();
}
