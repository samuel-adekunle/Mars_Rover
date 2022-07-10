import * as K from "../lib/constants";
import useWebSocket, { ReadyState } from "react-use-websocket";
import ConnectPage from "../components/connectPage";
import DashboardPage from "../components/dashboardPage";

export default function Index(): JSX.Element {
	const { readyState } = useWebSocket(K.Server.url, {
		shouldReconnect: () => true
	});

	return readyState !== ReadyState.OPEN ? <ConnectPage /> : <DashboardPage />;
}