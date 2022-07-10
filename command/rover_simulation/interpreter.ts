/* eslint-disable indent */
import WebSocket from "ws";
import fs from "fs";
import readline from "readline";
import { Payload } from "./schema";
import prompt from "prompt";
import * as api from "./commands";

export default class Interpreter {
	static ws: WebSocket
	static isActive = false
	private static schema: prompt.Schema = {
		properties: {
			command: {
				required: true,
				type: "string",
				pattern: /[ors]([xytcd]=[0-9xa-fA-F]+)*/,
				description: "Enter a command (o/r/s)",
				message: `Usage
To add an obstacle: ox={number}y={number}c={number} e.g. ox=10y=10c=0xFFF
To move rover: rx={number}y={number}t={number} e.g. rx=10y=10t=10
To set subsystems: s e.g. s`
			},
		}
	}

	static sendCommand(payload: Payload): void {
		payload.data = JSON.stringify(payload.data);
		Interpreter.ws.send(JSON.stringify(payload));
	}

	static async start(): Promise<void> {
		Interpreter.isActive = true;

		const rl = readline.createInterface({
			input: fs.createReadStream("./one-ball.txt")
		});

		for await (const line of rl) {
			if (!Interpreter.isActive) break;
			const _arr = line.split(",");
			const instr = _arr[0];
			const timeout = Number(_arr[1]);
			switch (instr[0]) {
				case "o": {
					const arr = instr.split("=");
					const x = Number(arr[1].slice(0, -1));
					const y = Number(arr[2].slice(0, -1));
					const color = Number(arr[3]);
					setTimeout(() => {
						Interpreter.sendCommand(api.obstacle({ x, y }, color));
					}, timeout);
					break;
				}
				case "r": {
					const arr = instr.split("=");
					const x = Number(arr[1].slice(0, -1));
					const y = Number(arr[2].slice(0, -1));
					const theta = Number(arr[3]);
					setTimeout(() => {
						Interpreter.sendCommand(api.rover({ x, y, theta }));
					}, timeout);
					break;
				}
				case "s": {
					setTimeout(() => {
						Interpreter.sendCommand(api.subsystem_status());
					}, timeout);
					break;
				}
				case "d": {
					const arr = instr.split("=");
					const x = Number(arr[1].slice(0, -1));
					const y = Number(arr[2]);
					setTimeout(async () => {
						Interpreter.sendCommand(api.destination({ x, y }));
					}, timeout);
				}
			}
		}
	}


	static stop(): void {
		Interpreter.isActive = false;
	}
}