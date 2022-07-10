/* eslint-disable indent */
import * as https from "https";
import * as http from "http";
import * as WebSocket from "ws";
import * as fs from "fs";


const port = process.env.PORT || 8000;
const mode = String("broadcast");
const server = port === 8000 ? http.createServer() : https.createServer({
	key: fs.readFileSync("./cert/fullchain.pem"),
	cert: fs.readFileSync("./cert/privkey.pem")
});
const wss = new WebSocket.Server({ server });
const ids = new Map<WebSocket, string>();

const echoHandler = (ws: WebSocket) => {
	ws.on("message", (message: string) => {
		console.log(`${new Date().toUTCString()}: (${ids.get(ws)}) ${message.trim()}`);
		ws.send(`${message}`);
	});
};

const broadcastHandler = (ws: WebSocket) => {
	ws.on("message", (message: string) => {
		console.log(`${new Date().toUTCString()}: (${ids.get(ws)}) ${message.trim()}`);
		wss.clients.forEach(client => {
			if (client !== ws) {
				client.send(`${message}`);
			}
		});
	});
};

const closeHandler = (ws: WebSocket) => {
	console.log(`${new Date().toUTCString()}: connection to id=${ids.get(ws)} closed`);
};

wss.on("connection", (ws: WebSocket) => {
	const id = Math.random().toString(36).substr(2, 10);
	switch (mode) {
		case "broadcast":
			broadcastHandler(ws);
			ids.set(ws, id);
			break;
		case "echo":
			echoHandler(ws);
			ids.set(ws, id);
			break;
		default:
			console.error(`${new Date().toUTCString()}: unknown mode: ${mode}`);
			process.exitCode = 1;
			return ws.close();
	}
	console.log(`${new Date().toUTCString()}: new connection with id=${id}`);
	ws.send(`id=${id}`);
});

wss.on("close", closeHandler);

server.listen(port);
