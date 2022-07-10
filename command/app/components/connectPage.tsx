import * as K from "../lib/constants";
import Head from "next/head";
import Image from "next/image";
import { useEffect } from "react";
import { useRouter } from "next/router";


function ConnectPage(): JSX.Element {
	const router = useRouter();
	useEffect(() => {
		router.push("/", "/connect", { shallow: true });
	}, []);

	return <>
		<Head>
			<title>Connect | Command Module</title>
			<link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png" />
			<link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png" />
			<link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png" />
			<link rel="manifest" href="/site.webmanifest" />
		</Head>
		<div className="container flex items-center justify-center bg-gray-900" style={{ height: "100vh" }}>
			<div className="text-center" style={{ maxWidth: "60vw" }}>
				<Image
					src={K.Images.rotatingSettingIcon.src}
					width={K.Images.rotatingSettingIcon.width / 1.5}
					height={K.Images.rotatingSettingIcon.height / 1.5}
				/>
				<div className="text-5xl text-white pt-4 pb-4">
					Welcome to the Mars Rover Command Module
				</div>
				<div className="text-3xl text-white pb-4">
					You are currently not connected to the server
				</div>
				<div className="text-3xl text-white pb-4">
					Please check that the server is turned on then try refreshing the page
				</div>
				<div className="text-3xl text-white pb-4">
					Server Address: {K.Server.url}
				</div>
			</div>
		</div>
	</>;
}

export default ConnectPage;