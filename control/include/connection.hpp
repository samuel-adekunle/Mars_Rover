#ifndef GUARD_CONNECTION_HPP
#define GUARD_CONNECTION_HPP

#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <SPI.h>
#include <LinkedList.h>
#include "../include/command_interpreter.hpp"
void commandHandler(websockets::WebsocketsMessage msg);

class Connections{
private:
	static constexpr unsigned int BALL_ORDER[5] = {0xFF0000,0xFFFF00,0x00FF00, 0x0000FF, 0xFF69B4};
	static SPIClass * vision_spi;
	static websockets::WebsocketsClient client;

	// static constexpr char* const SSID = "VM0869937"; 
	// static constexpr char* const PASSWORD = "JonahMichael1714"; 

	// static constexpr char* const SSID = "BTHub5-3XP6"; 
	// static constexpr char* const PASSWORD = "6f5b5de6ec"; 

	static constexpr char* const SSID = "BT-CNAL86"; 
	static constexpr char* const PASSWORD = "Karfakis!"; 

	// static constexpr char* const SSID = "49 Montrose"; 
	// static constexpr char* const PASSWORD = "f#MQGfNpq9J!"; 

	// static constexpr char* const SSID = "BT-6JAJ53"; 
	// static constexpr char* const PASSWORD = "36qGMLdgTbCM9q"; 

	static constexpr char * const SERVER_IP_PORT = "ws://192.168.1.195:8000/ws";
	static constexpr int spiClk = 1000000; // 1 MHz

	static bool WiFi_connected;
	static bool command_connected;
	static bool vision_connected;
	static bool energy_connected;
	static bool drive_connected;

	static constexpr int VISION_PING_RETURN_VAL = 170;
	static constexpr byte PING_MESSAGE = 0xFF;

	static bool connectToWiFi();
	static bool connectToCommand();


	static bool connectToVision();
	static bool connectToEnergy();
	static bool connectToDrive();
	
	static void pollVision(); 
	static void pollDrive();

public:
	static LinkedList<Move *> drive_tasks;

	static void setupAllConnections(bool require_WiFi_connection, bool require_command_connection);

	static bool getWiFiConnected()		{	return WiFi_connected;		};
	static bool getCommandConnected()	{	return command_connected;	};
	static bool getVisionConnected()	{	return vision_connected;	};
	static bool getEnergyConnected()	{	return energy_connected;	};
	static bool getDriveConnected()		{	return drive_connected;		};
	static bool transmitToCommand(const String& message);

	static void pollConnections();

	static bool stopOnSite;
	
	static constexpr int MAP_SCALING = 20;
	static constexpr int MAP_OFFSET = 250;
};

#endif */