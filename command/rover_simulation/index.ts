/* eslint-disable indent */
import * as api from "./commands";
import WebSocket from "ws";
import { Payload } from "./schema";
import Interpreter from "./interpreter";

const ws = new WebSocket("wss://rover-server.samuel-adekunle.software");
Interpreter.ws = ws;

ws.on("open", () => {
	console.log("Connected to server");
});

ws.on("message", (data: WebSocket.Data) => {
	try {
		const message: Payload = JSON.parse(data.toString());
		switch (message.class) {
			case "ping-command":
				Interpreter.sendCommand(api.pong());
				console.log("Connected to Command Module");
				break;
			case "start-command": {
				console.log("Rover started");
				if (!Interpreter.isActive) Interpreter.start();
				break;
			}
			case "stop-command": {
				console.log("\nRover stopped");
				Interpreter.stop();
				break;
			}
		}
	} catch (e) { /* DO NOTHING */ }
});