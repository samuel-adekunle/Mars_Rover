import { Payload, Position } from "./schema";

// const randomInt = (low: number, high: number): number => low + Math.floor(Math.random() * (high - low));

export const pong = (): Payload => ({
	class: "pong-command",
	timestamp: Date.now(),
	data: {}
});

export const obstacle = (position: Position, color: number): Payload => ({
	class: "obstacle",
	timestamp: Date.now(),
	data: {
		position,
		color: color
	}
});

export const subsystem_status = (): Payload => ({
	class: "subsystem-status",
	timestamp: Date.now(),
	data: {
		drive: true,
		energy: false,
		vision: true
	}
});

export const rover = (position: Position & { theta: number }): Payload => ({
	class: "rover",
	timestamp: Date.now(),
	data: {
		position,
		speed: 5,
		battery: 100,
		charging: true
	}
});

export const destination = (position: Position): Payload => ({
	class: "destination",
	timestamp: Date.now(),
	data: { position }
});