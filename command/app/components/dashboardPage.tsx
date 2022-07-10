import * as K from "../lib/constants";
import * as pathFinder from "../lib/pathFinder";
import * as rover from "../lib/rover";
import { Ball, ObstacleData, Payload, Position, RoverData, SVGConfig, SubsystemData, MoveData } from "../lib/schema";
import { useCallback, useEffect, useRef, useState } from "react";
import CommandButton from "../components/commandButton";
import Head from "next/head";
import PanSVG from "../lib/svg";
import { useRouter } from "next/router";
import useWebSocket from "react-use-websocket";

function DashboardPage(): JSX.Element {
	// Hooks
	const router = useRouter();
	const { sendMessage, lastMessage } = useWebSocket(K.Server.url, {
		shouldReconnect: () => true
	});

	// Refs
	const svgContainer = useRef<any>(null);

	// State
	const [roverBattery, setRoverBattery] = useState(0);
	const [roverCharging, setRoverCharging] = useState(false);
	const [roverSpeed, setRoverSpeed] = useState(0);
	const [isRoverStarted, setIsRoverStarted] = useState(false);
	const [websocketId, setWebsocketId] = useState("");
	const [visionSystem, setVisionSystem] = useState(false);
	const [driveSystem, setDriveSystem] = useState(false);
	const [energySystem, setEnergySystem] = useState(false);
	const [pingId, setPingId] = useState<NodeJS.Timeout>();
	const [isConnected, setIsConnected] = useState(false);
	const [svgConfig, setSVGConfig] = useState<SVGConfig>({
		darkMode: true,
		displayRover: true,
		displayBallPadding: true,
		displayVisitedTiles: true,
		displayQueuedTiles: true
	});
	const [endpoint, setEndPoint] = useState<Position | null>(null);
	const [mapSVG, setMapSVG] = useState("");
	const [messages, setMessages] = useState<Array<Payload & { latency: number }>>([]);
	const [latency, setLatency] = useState({ avg: 0, n: 0 });
	const [darkMode, setDarkMode] = useState(true);
	const [svgCursorType, setSVGCursorType] = useState("grab");

	// Methods
	const handleSendReset = useCallback(() => rover.sendReset(sendMessage), [sendMessage]);
	const handleSendScan = useCallback(() => rover.sendScan(sendMessage), [sendMessage]);
	const handleSendPing = useCallback(() => rover.sendPing(sendMessage), [sendMessage]);
	const handleSendPong = useCallback(() => rover.sendPong(sendMessage), [sendMessage]);

	const handleSendStart = useCallback(() => {
		rover.sendStart(sendMessage);
		setIsRoverStarted(true);
	}, [sendMessage]);

	const handleSendStop = useCallback(() => {
		rover.sendStop(sendMessage);
		setIsRoverStarted(false);
	}, [sendMessage]);


	const handleReconnect = () => {
		setPingId(setInterval(handleSendPing, K.Server.pingInterval));
		setIsConnected(false);
	};

	const handleSendMove = useCallback((pos: Position) => {
		rover.sendMove(sendMessage, pos);
	}, [sendMessage]);

	const handleSendClearPath = useCallback(() => {
		rover.sendPath(sendMessage);
	}, [sendMessage]);

	const handleSetEndpoint = async () => {
		setSVGCursorType((currentCursor) => currentCursor == "grab" ? "pointer" : "grab");
	};

	const handleExactlySetEndpoint = async () => {
		let x: any = window.prompt("Enter x coordinate: ");
		if (!x) return;
		let y: any = window.prompt("Enter y coordinate: ");
		if (!y) return;
		x = Number(x);
		y = Number(y);
		setEndPoint({ x, y });
		await calculatePath({ x, y });
		await renderSVG();
	};

	const handleClickSVG = async () => {
		if (svgCursorType == "grab") return;
		const point = PanSVG.clickedPoint;
		setEndPoint(point);
		await calculatePath(point);
		await renderSVG();
	};

	const renderSVG = async () => {
		const svg = await pathFinder.svg(svgConfig);
		setMapSVG(svg);
		PanSVG.svg = document.querySelector("svg");
		PanSVG.addEventListeners();
		PanSVG.setViewBox();
	};

	const calculatePath = async (position: Position) => {
		const pos = await pathFinder.findPath(position);
		handleSendClearPath();
		pos.slice(0, -1).forEach((_pos) => handleSendMove(_pos));
		if (isRoverStarted) handleSendStart();
	};

	const handleResetPathFinder = async () => {
		await pathFinder.reset();
		await renderSVG();
	};

	const handleDeletePath = async () => {
		await pathFinder.deletePath();
		await renderSVG();
	};

	const addObstacleToSVG = async (ball: Ball) => {
		await pathFinder.ball(ball);
		if (endpoint) await calculatePath(endpoint);
		await renderSVG();
	};

	const setSVGRoverPosition = async (position: Position & { theta: number }) => {
		await pathFinder.setRoverPosition(position);
		await renderSVG();
	};

	const handleClearMessages = () => {
		setMessages([]);
	};

	const handleToggleDarkMode = () => {
		setDarkMode((currentMode) => !currentMode);
		setSVGConfig((currentConfig) => ({
			...currentConfig,
			darkMode: !currentConfig.darkMode
		}));
	};

	const handleToggleDisplayBallPadding = () => {
		setSVGConfig((currentConfig) => ({
			...currentConfig,
			displayBallPadding: !currentConfig.displayBallPadding
		}));
	};

	const handleToggleDisplayQueuedTiles = () => {
		setSVGConfig((currentConfig) => ({
			...currentConfig,
			displayQueuedTiles: !currentConfig.displayQueuedTiles
		}));
	};

	const handleToggleDisplayRover = () => {
		setSVGConfig((currentConfig) => ({
			...currentConfig,
			displayRover: !currentConfig.displayRover
		}));
	};

	const handleToggleDisplayVisitedTiles = () => {
		setSVGConfig((currentConfig) => ({
			...currentConfig,
			displayVisitedTiles: !currentConfig.displayVisitedTiles
		}));
	};

	const formatMessage = (message: Payload & { latency: number }): string => {
		let prettyMessage = "";
		switch (message.class) {
			case "pong-command":
				prettyMessage = "received reply from rover";
				break;
			case "rover":
				prettyMessage = `pos=(${message.data.position.x}, ${message.data.position.y}, ${message.data.position.theta}°), battery=${message.data.battery}%, charging=${message.data.charging}, speed=${message.data.speed}`;
				break;
			case "obstacle":
				prettyMessage = `pos=(${message.data.position.x}, ${message.data.position.y}), color=${message.data.color}`;
				break;
			case "subsystem-status":
				prettyMessage = `energy=${message.data.energy}, drive=${message.data.drive}, vision=${message.data.vision}`;
		}
		return `[${message.timestamp}] ${message.class}: ${prettyMessage} (${message.latency} ms)`;
	};

	// Lifecycle Events
	useEffect(() => {
		router.push("/", "/dashboard", { shallow: true });
		handleReconnect();

		if (svgContainer.current) {
			PanSVG.containerSize.width = svgContainer.current.clientWidth;
			PanSVG.containerSize.height = svgContainer.current.clientHeight;
		}
		renderSVG();
	}, []);

	useEffect(() => {
		renderSVG();
	}, [svgConfig]);

	useEffect(() => {
		try {
			const __message = JSON.parse(lastMessage?.data);
			__message.data = JSON.parse(__message.data);
			const message: Payload = __message;
			const latency = Date.now() - message.timestamp;
			setLatency(({ avg, n }) => ({ avg: (avg * n + latency) / (n + 1), n: n + 1 }));

			setMessages(_messages => [{ ...message, latency }, ..._messages]);
			switch (message.class) {
				case "pong-command": {
					setIsConnected(true);
					if (pingId) clearInterval(pingId);
					break;
				}
				case "ping-command": {
					handleSendPong();
					break;
				}
				case "rover": {
					const data: RoverData = message.data;
					setRoverSpeed(data.speed);
					setRoverBattery(data.battery);
					setRoverCharging(data.charging);
					setSVGRoverPosition(data.position);
					break;
				}
				case "subsystem-status": {
					const data: SubsystemData = message.data;
					setEnergySystem(data.energy);
					setDriveSystem(data.drive);
					setVisionSystem(data.vision);
					break;
				}
				case "obstacle": {
					const data: ObstacleData = message.data;
					addObstacleToSVG({ pos: data.position, colour: data.color });
					break;
				}
				case "destination": {
					const data: MoveData = message.data;
					setEndPoint(data.position);
					calculatePath(data.position);
					renderSVG();
					break;
				}
				default: {
					console.error(`Unexpected payload class: ${message.class}`);
				}
			}
		} catch (e) {
			if (String(lastMessage?.data).slice(0, 3) === "id=") {
				setWebsocketId(String(lastMessage?.data).slice(3));
			} else {
				console.error(`unexpected message: ${lastMessage?.data}`);
			}
		}
	}, [lastMessage]);

	return <>
		<Head>
			<title>Dashboard | Command Module</title>
			<link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png" />
			<link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png" />
			<link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png" />
			<link rel="manifest" href="/site.webmanifest" />
		</Head>
		<div className={`${darkMode ? "bg-gray-900" : "bg-gray-400"} grid grid-rows-6 gap-2 p-2`} style={{ height: "100vh" }}>
			<div className="row-span-4 grid grid-cols-7 gap-2">
				<div className={`${darkMode ? "bg-gray-800" : "bg-gray-300"}  col-span-5 p-2 grid grid-rows-1`}>
					<div ref={svgContainer} className="row-span-1">
						<object
							style={{
								cursor: svgCursorType
							}}

							onClick={handleClickSVG}

							dangerouslySetInnerHTML={{
								__html: mapSVG
							}}>
						</object>
					</div>
				</div>
				<div className={`${darkMode ? " bg-gray-800" : "bg-gray-300"} col-span-2`}>
					<div className={`${darkMode ? " text-white" : "text-black"} text-center text-3xl font-bold mt-2 mb-2`}>Commands</div>
					<div className="pl-2 pr-2 text-center">
						{isConnected && <>
							{!isRoverStarted
								? <CommandButton darkMode={darkMode} text={"Start Rover"} onClick={handleSendStart} />
								: <CommandButton darkMode={darkMode} text={"Stop Rover"} onClick={handleSendStop} />
							}
							<CommandButton darkMode={darkMode} text={"Reset Rover"} onClick={handleSendReset} />
							<CommandButton darkMode={darkMode} text={"Scan Environment"} onClick={handleSendScan} />
							<CommandButton darkMode={darkMode} text={"Refresh Connection"} onClick={handleReconnect} />
						</>}
						<CommandButton darkMode={darkMode} text={"Clear Path"} onClick={handleDeletePath} />
						<CommandButton darkMode={darkMode} text={"Reset Map"} onClick={handleResetPathFinder} />
						<CommandButton darkMode={darkMode} text={"Set Destination"} onClick={handleExactlySetEndpoint} />
						<CommandButton darkMode={darkMode} text={svgCursorType == "grab" ? "Enable Destination Picker" : "Disable Destination Picker"} onClick={handleSetEndpoint} />
						<CommandButton darkMode={darkMode} text={darkMode ? "Light Mode" : "Dark Mode"} onClick={handleToggleDarkMode} />
						<CommandButton darkMode={darkMode} text={svgConfig.displayRover ? "Hide Rover" : "Show Rover"} onClick={handleToggleDisplayRover} />
						<CommandButton darkMode={darkMode} text={svgConfig.displayBallPadding ? "Hide Padding" : "Show Padding"} onClick={handleToggleDisplayBallPadding} />
						<CommandButton darkMode={darkMode} text={svgConfig.displayQueuedTiles ? "Hide Queued Tiles" : "Show Queued Tiles"} onClick={handleToggleDisplayQueuedTiles} />
						<CommandButton darkMode={darkMode} text={svgConfig.displayVisitedTiles ? "Hide Visited Tiles" : "Show Visited Tiles"} onClick={handleToggleDisplayVisitedTiles} />
					</div>
				</div>
			</div>
			<div className="row-span-2 grid grid-cols-10 gap-2">
				<div className={`col-span-2 ${darkMode ? " bg-gray-800" : "bg-gray-300"}`}>
					<div className={`text-3xl font-bold text-center ${darkMode ? " text-white" : "text-black"} mt-2 mb-2`}>
						Connection
					</div>
					<div className={`${darkMode ? "text-white" : "text-black"} text-2xl pl-2`}>
						<div>Server: Connected</div>
						<div>Socket Id: {websocketId}</div>
						<div>Rover: {isConnected ? "Connected" : "Not Connected"}</div>
						<div>Latency: {Math.round(latency.avg)} ms</div>
					</div>
				</div>
				<div className={`col-span-2 ${darkMode ? "bg-gray-800" : "bg-gray-300"}`}>
					<div className={`text-3xl font-bold text-center ${darkMode ? "text-white" : "text-black"} mt-2 mb-2`}>
						Rover
					</div>
					<div className={`${darkMode ? "text-white" : "text-black"} text-2xl pl-2`}>
						<div>Speed: {roverSpeed} cm/s</div>
						<div>Battery: {roverBattery}% {roverCharging ? "Charging" : "Not Charging"}</div>
						<div>Energy Module: {energySystem ? "Operational" : "Not Operational"}</div>
						<div>Drive Module: {driveSystem ? "Operational" : "Not Operational"}</div>
						<div>Vision Module: {visionSystem ? "Operational" : "Not Operational"}</div>
					</div>
				</div>
				<div className={`col-span-6 ${darkMode ? "bg-gray-800" : "bg-gray-300"} overflow-y-auto`}>
					<div className="grid grid-cols-5">
						<div className={`col-span-4 text-3xl font-bold text-center ${darkMode ? "text-white" : "text-black"} mt-2 mb-2`}>
							Message History
						</div>
						<span className="text-right col-span-1 pr-2">
							<span style={{ maxWidth: "150px" }}>
								<CommandButton darkMode={darkMode} text={"Clear"} padding={2} onClick={handleClearMessages} />
							</span>
						</span>
					</div>
					<div className={`pl-2 ${darkMode ? "text-white" : "text-black"} text-2xl`}>
						{messages.map((message, idx) => <div key={idx}>{formatMessage(message)}</div>)}
					</div>
				</div>
			</div>
		</div>
	</>;
}

export default DashboardPage;

