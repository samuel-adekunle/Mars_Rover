"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    Object.defineProperty(o, k2, { enumerable: true, get: function() { return m[k]; } });
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
/* eslint-disable indent */
const http = __importStar(require("http"));
const WebSocket = __importStar(require("ws"));
const port = process.env.PORT || 8000;
const mode = String("broadcast");
const server = http.createServer();
const wss = new WebSocket.Server({ server });
const ids = new Map();
const echoHandler = (ws) => {
    ws.on("message", (message) => {
        console.log(`${new Date().toUTCString()}: (${ids.get(ws)}) ${message.trim()}`);
        ws.send(`${message}`);
    });
};
const broadcastHandler = (ws) => {
    ws.on("message", (message) => {
        console.log(`${new Date().toUTCString()}: (${ids.get(ws)}) ${message.trim()}`);
        wss.clients.forEach(client => {
            if (client !== ws) {
                client.send(`${message}`);
            }
        });
    });
};
const closeHandler = (ws) => {
    console.log(`${new Date().toUTCString()}: connection to id=${ids.get(ws)} closed`);
};
wss.on("connection", (ws) => {
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
