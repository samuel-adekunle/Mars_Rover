#ifndef GUARD_COMMAND_INTERPRETER_HPP
#define GUARD_COMMAND_INTERPRETER_HPP

#include <ArduinoJson.h>

using Colour = uint32_t;
struct Sendable{
	constexpr static int PAYLOAD_JSON_SIZE = 1536;
	virtual String getClassName() = 0;
	virtual void toJSON(String &output) = 0;
	void send();
};
struct Subsystem_Status : public Sendable{
	static constexpr int SUBSYSTEM_STATUS_JSON_SIZE = 128;
	bool vision;
	bool drive;
	bool energy;
	String getClassName();
	Subsystem_Status();
	Subsystem_Status(bool vision, bool drive, bool energy);
	void toJSON(String & output);
};

struct Obstacle : public Sendable{
	static constexpr int OBSTACLE_JSON_SIZE = 128;
	int x, y;
	Colour colour;

	Obstacle() = default;
	Obstacle(String& json_obstacle);
	void toJSON(String & output);
	String getClassName();
};

struct Pong : public Sendable{
	Pong() = default;
	void toJSON(String & output);
	String getClassName();
};

struct Move : public Sendable {
	Move() = default;
	Move(const String &output);
	Move(const Move&) = default;
	int x, y;
	void toJSON(String &output);
	String getClassName();
	void sendToRover() const;
};

struct Rover : public Sendable {
	static constexpr int ROVER_JSON_SIZE = 192;
	Rover() = default;
	int x, y, theta;
	int battery;
	int speed;
	bool charging;
	void toJSON(String &output);
	String getClassName();
	
};


struct Payload{
	constexpr static int PAYLOAD_JSON_SIZE = 1536;
	String payload_type;
	unsigned long timestamp;
	String data;
};

#endif