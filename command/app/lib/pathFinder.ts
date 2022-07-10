import * as K from "./constants";
import { Ball, Position, SVGConfig } from "./schema";

export const ball = async (ball: Ball): Promise<void> => {
	const req: RequestInit = {
		method: "POST",
		body: JSON.stringify(ball),
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	await fetch(K.PathFinder.endpoint("ball"), req);
};

export const reset = async (): Promise<void> => {
	const req: RequestInit = {
		method: "POST",
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	await fetch(K.PathFinder.endpoint("reset"), req);
};

export const setRoverPosition = async (position: Position & { theta: number }): Promise<void> => {
	const req: RequestInit = {
		method: "POST",
		body: JSON.stringify({
			pos: {
				x: position.x,
				y: position.y
			},
			theta: position.theta
		}),
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	await fetch(K.PathFinder.endpoint("roverPosition"), req);
};

export const findPath = async (endpoint: Position): Promise<Array<Position>> => {
	const req: RequestInit = {
		method: "POST",
		body: JSON.stringify(endpoint),
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	const res = await fetch(K.PathFinder.endpoint("findPath"), req);
	const data = await res.json();
	return data;
};

export const deletePath = async (): Promise<void> => {
	const req: RequestInit = {
		method: "POST",
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	await fetch(K.PathFinder.endpoint("deletePath"), req);
};

export const svg = async (config: SVGConfig): Promise<string> => {
	const req: RequestInit = {
		method: "POST",
		body: JSON.stringify(config),
		headers: {
			Origin: "http://localhost:3000"
		}
	};
	const res = await fetch(K.PathFinder.endpoint("svg"), req);
	const data = await res.json();
	return data.out;
};