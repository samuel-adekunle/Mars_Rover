#include "../include/connection.hpp"
#include "../include/vision_interpreter.hpp"
#include "../include/command_interpreter.hpp"
#include "../include/drive_interpreter.hpp"

#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_SCLK   14
#define HSPI_SS     15
#define SPI_INTERRUPT 4

bool Connections::WiFi_connected = false;
bool Connections::command_connected = false;
bool Connections::vision_connected = false;
bool Connections::energy_connected = false;
bool Connections::drive_connected = false;
SPIClass * Connections::vision_spi = nullptr;
websockets::WebsocketsClient Connections::client;

LinkedList<Move *> Connections::drive_tasks = LinkedList<Move *>();
constexpr unsigned int Connections::BALL_ORDER[5];

bool Connections::stopOnSite = false;


bool Connections::connectToWiFi(){
	WiFi.begin(SSID, PASSWORD);

	//Wait some time to connect to wifi
	for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
		delay(1000);
	}
	return WiFi.status()==WL_CONNECTED;
}

bool Connections::connectToCommand(){
	client.close();
	client.onMessage(commandHandler);

	for(int i = 0; i < 5; i++){
		if(client.connect(SERVER_IP_PORT)){
			return true;
		}
		delay(100);
	}
	return false;
}

bool Connections::transmitToCommand(const String &message){

	if(command_connected){
		client.send(message);
	}
	return command_connected;
}


void Connections::pollVision(){
	static int oldX[5];
	static int oldY[5];
	if(vision_connected){
		pinMode(HSPI_SS, OUTPUT);

		byte r[40];

		vision_spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
		digitalWrite(HSPI_SS, LOW);
		for(int i = 0; i < 40; i++){
			r[i] = vision_spi->transfer(0x00);
		}


		digitalWrite(HSPI_SS, HIGH);
		vision_spi->endTransaction();

		bool clearPath = true;
		std::pair<float,float> balls[5];
		//R Y G B P
		for(byte i = 0; i < 5; i++){
			uint16_t x1 = r[0 +i*8];
			x1 <<=8;
			x1 += r[1 + i*8];

			uint16_t y1 = r[2 + i*8];
			y1 <<= 8;
			y1 += r[3 + i*8];
			
			uint16_t x2 = r[4 + i*8];
			x2 <<= 8;
			x2 += r[5 + i*8];

			uint16_t y2 = r[6 + i*8];
			y2 <<= 8;
			y2 += r[7 + i*8];

			if(x1 == 0x27f && y1 == 0x1df && x2 == 0 && y2 == 0){
				continue;
			} 
			unsigned int bounds[] = {x1,y1,x2,y2};
			double distance = get_distance(bounds) * Connections::MAP_SCALING;
			double angle = get_angle(bounds);
			
			if(distance < 100 and distance > 0 && (i == 0|| i == 2)) {
				Obstacle obs;
				obs.colour = BALL_ORDER[i];
				Drive::getPositionFromRelativePolar(distance, angle, obs.x, obs.y);
				if(oldX[i] != 0){
					obs.x = oldX[i] * 0.9 + obs.x * 0.1;
					oldX[i] = obs.x;
				}
				else{
					oldX[i] = obs.x;
				}
				if(oldY[i] != 0){
					obs.y = oldY[i] * 0.9 + obs.y * 0.1;
					oldY[i] = obs.y;
				}
				else{
					oldY[i] = obs.y;
				}
				obs.send();
				clearPath = false;
			}
			balls[i] = std::pair<double, double>(distance, angle);
		}
		if(clearPath && Connections::stopOnSite){
			if( Connections::drive_tasks.size() == 0){
				Move * move = new Move();
				move->x = Drive::x+5;
				move->y =  Drive::y+5;
				Connections::drive_tasks.add(move);
				Serial2.println("f"+String(move->x)+","+String(move->y)+"?");
			}
		}
		if(!clearPath && Connections::stopOnSite){
			if( Connections::drive_tasks.size() != 0){
				Serial2.println("d?");
				delete Connections::drive_tasks.remove(0);
			}
			Connections::stopOnSite = false;
		}

		// Serial.println("RED	: " + String(balls[0][0])+ " " + String(balls[0][1]));
		// Serial.println("YELLOW 	: " + String(balls[1][0])+ " " + String(balls[1][1]));
		// Serial.println("GREEN	: " + String(balls[2][0])+ " " + String(balls[2][1]));
		// Serial.println("BLUE	: " + String(balls[3][0])+ " " + String(balls[3][1]));
		// Serial.println("PINK	: " + String(balls[4][0])+ " " + String(balls[4][1]));

	}
}

bool Connections::connectToVision(){
	if(vision_spi){
		delete vision_spi;
	}
	vision_spi = new SPIClass(HSPI);
	vision_spi->begin();
	pinMode(HSPI_SS, OUTPUT);

	byte r[8];

	vision_spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
	digitalWrite(HSPI_SS, LOW);
	r[0] = vision_spi->transfer(0x00);
	r[1] = vision_spi->transfer(0x00);
	r[2] = vision_spi->transfer(0x00);
	r[3] = vision_spi->transfer(0x00);
	r[4] = vision_spi->transfer(0);
	r[5] = vision_spi->transfer(0);
	r[6] = vision_spi->transfer(0);
	r[7] = vision_spi->transfer(0);

	digitalWrite(HSPI_SS, HIGH);
	vision_spi->endTransaction();

	for(byte b : r){
		if(b != 0){
			return true;
		}
	}
	return false;
}

bool Connections::connectToEnergy(){
	return false;
}
void Connections::pollDrive(){
	static unsigned int last_position_check = 0;
	if(drive_connected){
		if(Serial2.available()){
			String val = Serial2.readStringUntil('?');
			Serial.println(val);
			if(val == "d"){
				if( Connections::drive_tasks.size() != 0){
					delete Connections::drive_tasks.remove(0);
					if( Connections::drive_tasks.size() != 0){
						const Move * const move = Connections::drive_tasks.get(0);
						move->sendToRover();
					}
				}
			}
			if(val[0] == 'a'){
				int start = 1;
				int index = 0;

				int param[2];

				for (int i = 1; (i < val.length()) && (index < 3); i++)
				{
					if (val[i] == ',')
					{
						param[index++] = (val.substring(start, i)).toInt();
						start = i + 1;
					}
					else if(i==val.length() - 1){
						param[index++] = (val.substring(start, i+1)).toInt();
					}
				}
				Drive::x = param[0];
				Drive::y = param[1];
				Rover rover;
				rover.x = Drive::x;
				rover.y = Drive::y;
				rover.send();
			}
		}
		if(millis() - last_position_check > 1000){
			Serial2.print("a?");
			last_position_check = millis();
		}
	}
}
bool Connections::connectToDrive(){
	Serial2.println("e?");
	long start = millis();
	constexpr unsigned long TIMEOUT = 2000;
	while(millis() - start < TIMEOUT){
		if(Serial2.available()){
			String val = Serial2.readStringUntil('?');
			if(val== "e"){
				return true;
			}
			return false;
		}
	}
	drive_connected = false;
	return false;
}

void Connections::setupAllConnections(bool require_WiFi_connection, bool require_command_connection){
	Serial.println(R"(    __  ___ ___     ____  _____    ____   ____  _    __ ______ ____ )");
	Serial.println(R"(   /  |/  //   |   / __ \/ ___/   / __ \ / __ \| |  / // ____// __ \)");
	Serial.println(R"(  / /|_/ // /| |  / /_/ /\__ \   / /_/ // / / /| | / // __/  / /_/ /)");
	Serial.println(R"( / /  / // ___ | / _, _/___/ /  / _, _// /_/ / | |/ // /___ / _, _/ )");
	Serial.println(R"(/_/  /_//_/  |_|/_/ |_|/____/  /_/ |_| \____/  |___//_____//_/ |_|  )");
	Serial.println(R"(                                                                    )");
	do{
		WiFi_connected = connectToWiFi();
		if(require_WiFi_connection && !WiFi_connected){ 
			Serial.println("WiFI    Connection - FAILED!");
			Serial.println("Trying Again in 1 second");
			delay(1000);
		}
	}while(!WiFi_connected && require_WiFi_connection);
	do{
		command_connected = connectToCommand();
		if(require_command_connection && !command_connected){
			Serial.println("Command Connection - FAILED!");
			Serial.println("Trying Again in 1 second"); 
			delay(1000);
		}
	}while(!command_connected && require_command_connection);

	vision_connected = connectToVision();
	energy_connected = connectToEnergy();
	drive_connected = connectToDrive();

	Serial.println((WiFi_connected) ? "WiFi    Connection - SUCCESSFUL!"  : "WiFi    Connection - FAILED!"); 
	Serial.println((command_connected) ? "Command Connection - SUCCESSFUL!"  : "Command Connection - FAILED!"); 
	Serial.println((vision_connected) ? "Vision  Connection - SUCCESSFUL!"  : "Vision  Connection - FAILED!"); 
	Serial.println((energy_connected) ? "Energy  Connection - SUCCESSFUL!"  : "Energy  Connection - FAILED!"); 
	Serial.println((drive_connected) ?  "Drive   Connection - SUCCESSFUL!"  : "Drive   Connection - FAILED!"); 
	Serial.print("\n> ");
	Serial.flush();

}


void Connections::pollConnections(){
	client.poll();
	pollVision();
	pollDrive();
	delay(100);
}