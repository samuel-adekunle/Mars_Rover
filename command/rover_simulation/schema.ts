// Base Types

export type Position = {
	x: number,
	y: number
}

export type Color = number

export type Battery = number

export type Charging = boolean

export type Speed = number

export type Vision = boolean

export type Drive = boolean

export type Energy = boolean

// Payload Data Types

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface StartData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface StopData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface ResetData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface ScanData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface PathData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface PingData { }

// eslint-disable-next-line @typescript-eslint/no-empty-interface
interface PongData { }

export interface MoveData {
	position: Position
}

export interface ObstacleData {
	position: Position,
	color: Color
}

export interface RoverData {
	position: Position & { theta: number },
	battery: Battery,
	charging: Charging,
	speed: Speed
}

export interface SubsystemData {
	vision: Vision,
	drive: Drive,
	energy: Energy
}

export type CommonPayload = {
	timestamp: number
}

type StartPayload = {
	class: "start-command",
	data: StartData
}

type StopPayload = {
	class: "stop-command",
	data: StopData
}

type ResetPayload = {
	class: "reset-command",
	data: ResetData
}

type ScanPayload = {
	class: "scan-command",
	data: ScanData
}

type PingPayload = {
	class: "ping-command",
	data: PingData
}

type PongPayload = {
	class: "pong-command",
	data: PongData
}

type MovePayload = {
	class: "move-command",
	data: string
}

type PathPayload = {
	class: "clear-path",
	data: PathData
}

export type ObstaclePayload = {
	class: "obstacle",
	data: ObstacleData
}

export type RoverPayload = {
	class: "rover",
	data: RoverData
}

type SubsystemPayload = {
	class: "subsystem-status",
	data: SubsystemData
}

// Final Payload type
export type Payload = CommonPayload
	& (StartPayload | StopPayload | ResetPayload | ScanPayload | PingPayload | PongPayload
	| MovePayload | ObstaclePayload | RoverPayload | SubsystemPayload | PathPayload | { class: "destination", data: MoveData })

// Path Finder types
export interface Ball {
	pos: Position,
	colour: Color
}

export interface SVGConfig {
	displayRover: boolean,
	displayBallPadding: boolean,
	displayVisitedTiles: boolean,
	displayQueuedTiles: boolean,
	darkMode: boolean
}