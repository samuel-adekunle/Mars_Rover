interface Image {
	src: string,
	width: number,
	height: number
}

export class Images {
	static get rotatingSettingIcon(): Image {
		return { src: "/settings.svg", width: 512, height: 512 };
	}
}

export class Server {
	static get port(): number {
		return 8000;
	}
	static get domain(): string {
		return "ws://localhost";
	}
	static get url(): string {
		return `${this.domain}:${this.port}`;
	}
	static get pingInterval(): number {
		return 3000;
	}
}

export class PathFinder {
	static get port(): number {
		return process.env.NODE_ENV === "production" ? 443 : 6900;
	}
	static get domain(): string {
		return process.env.NODE_ENV === "production" ? "https://samuel-cors-proxy.herokuapp.com/https://rover-path-finder.herokuapp.com" : "http://localhost";
	}
	static get url(): string {
		return `${this.domain}:${this.port}`;
	}
	static endpoint(name: string): string {
		return `${this.url}/${name}`;
	}
}