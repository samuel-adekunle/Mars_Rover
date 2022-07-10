#include "../include/command_interpreter.hpp"
#include "../include/connection.hpp"
#include <LinkedList.h>

Obstacle::Obstacle(String& json_obstacle){
	StaticJsonDocument<OBSTACLE_JSON_SIZE> doc;

	DeserializationError error = deserializeJson(doc, json_obstacle);

	if (error) {
		Serial.println("ERROR - Failed to deserialise incoming obstacle JSON");
		return;
	}

	x = doc["obstacle"]["position"]["x"];
	y = doc["obstacle"]["position"]["y"];
	
	int colour_int = doc["obstacle"]["color"];
	colour = static_cast<Colour>(colour_int);
}
String Obstacle::getClassName(){
	return "obstacle";
}


void Obstacle::toJSON(String & output){
	StaticJsonDocument<OBSTACLE_JSON_SIZE> doc;
	JsonObject obstacle_position = doc.createNestedObject("position");
	obstacle_position["x"] = x;
	obstacle_position["y"] = y;
	doc["color"] = static_cast<int>(colour);

	serializeJson(doc, output);
}

Subsystem_Status::Subsystem_Status() : vision(Connections::getVisionConnected()), drive(Connections::getDriveConnected()), energy(Connections::getEnergyConnected()) {}
Subsystem_Status::Subsystem_Status(bool vision, bool drive, bool energy) : vision(vision), drive(drive), energy(energy) {}
void Subsystem_Status::toJSON(String & output){
	StaticJsonDocument<SUBSYSTEM_STATUS_JSON_SIZE> doc;

	doc["vision"] = Connections::getVisionConnected();
	doc["drive"] = Connections::getDriveConnected();
	doc["energy"] = Connections::getEnergyConnected();

	serializeJson(doc, output);
}
String Subsystem_Status::getClassName(){
	return "subsystem-status";
}

void Pong::toJSON(String & output){
	output = "{}";
	return;
}
String Pong::getClassName(){
	return "pong-command";
}

Move::Move(const String &input){
	Serial.println(input);

	StaticJsonDocument<96> doc;

	DeserializationError error = deserializeJson(doc, input);

	if (error) {
		Serial.print(F("deserializeJson() failed: MOVE" ));
		Serial.println(error.f_str());
		return;
	}

	this->x = doc["position"]["x"]; // "int"
	this->y = doc["position"]["y"]; // "int"


}
void Move::toJSON(String &output){

}
String Move::getClassName(){
	return "move-command";
}

void Move::sendToRover() const{
	Serial.println(String("f")+String(this->x*Connections::MAP_SCALING - Connections::MAP_OFFSET)+String(",")+String(-this->y*Connections::MAP_SCALING + Connections::MAP_OFFSET)+String(",2,?"));
	Serial2.print(String("f")+String(this->x*Connections::MAP_SCALING - Connections::MAP_OFFSET)+String(",")+String(-this->y*Connections::MAP_SCALING + Connections::MAP_OFFSET)+String(",2,?"));
}


void commandHandler(websockets::WebsocketsMessage msg){
	StaticJsonDocument<Sendable::PAYLOAD_JSON_SIZE> doc;
	DeserializationError error = deserializeJson(doc, msg.data());

	if (error) {
		Serial.print(F("deserializeJson() failed: "));
		return;
	}

	const char* payload_class_cstr = doc["class"]; // "string"
	const char* payload_timestamp_cstr = doc["timestamp"]; // "string"
	const char* payload_data_cstr = doc["data"]; // "interface{}"
	
	String payload_class = payload_class_cstr;
	String payload_timestamp = payload_timestamp_cstr;
	String payload_data = payload_data_cstr;
	Serial.println(payload_class);
	if(payload_class == "ping-command"){
		//Send pong command
		Pong p;
		p.send();
		Subsystem_Status status;
		status.send();
	}
	else if(payload_class == "clear-path"){
		Serial2.print("d?");
		int listSize = Connections::drive_tasks.size();
		for(int i = 0; i < listSize; i++){
			delete Connections::drive_tasks.remove(0);
		}
	}
	else if(payload_class == "move-command" ){
		Move *move = new Move(payload_data);
		Connections::drive_tasks.add(move);
	}
	else if(payload_class == "start-command"){
		Serial.println(Connections::drive_tasks.size());
		if(Connections::drive_tasks.size() != 0 ){
			const Move * const move = Connections::drive_tasks.get(0);
			move->sendToRover();

		}
	}
	else if(payload_class == "stop-command"){
		//send message to drive to stop
		Serial2.print("d?");
	}
	else if(payload_class == "reset-command"){
		//send message to drive to stop
		Serial2.print("d?");
		ESP.restart();
	}
	else{
		Serial.println("Unused JSON command received");
	}
}
void Sendable::send(){
	String dataJSON;
	this->toJSON(dataJSON);

	StaticJsonDocument<PAYLOAD_JSON_SIZE> doc;

	doc["class"] = this->getClassName();
	doc["timestamp"] = millis();
	doc["data"] = dataJSON;

	String payloadJSON;
	serializeJson(doc, payloadJSON);
	Connections::transmitToCommand(payloadJSON);
}


void Rover::toJSON(String &output){
	StaticJsonDocument<ROVER_JSON_SIZE> doc;

	JsonObject position = doc.createNestedObject("position");
	position["x"] = this->x/Connections::MAP_SCALING + Connections::MAP_OFFSET/Connections::MAP_SCALING;
	position["y"] = -this->y/Connections::MAP_SCALING + Connections::MAP_OFFSET/Connections::MAP_SCALING;
	position["theta"] = this->theta;
	doc["battery"] = 100;
	doc["charging"] = false;
	doc["speed"] = 2;

	serializeJson(doc, output);
}

String Rover::getClassName(){
	return "rover";
}

