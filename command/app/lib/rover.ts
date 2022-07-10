import { Payload, Position } from "./schema";
import { SendMessage } from "react-use-websocket";

// Sent from Command Module

export const sendStart = (send: SendMessage): void => {
	const payload: Payload = {
		class: "start-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendStop = (send: SendMessage): void => {
	const payload: Payload = {
		class: "stop-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendReset = (send: SendMessage): void => {
	const payload: Payload = {
		class: "reset-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendScan = (send: SendMessage): void => {
	const payload: Payload = {
		class: "scan-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendMove = (send: SendMessage, position: Position): void => {
	const payload: Payload = {
		class: "move-command",
		timestamp: Date.now(),
		data: JSON.stringify({ position })
	};
	send(JSON.stringify(payload));
};

export const sendPing = (send: SendMessage): void => {
	const payload: Payload = {
		class: "ping-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendPong = (send: SendMessage): void => {
	const payload: Payload = {
		class: "pong-command",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};

export const sendPath = (send: SendMessage): void => {
	const payload: Payload = {
		class: "clear-path",
		timestamp: Date.now(),
		data: {}
	};
	send(JSON.stringify(payload));
};
