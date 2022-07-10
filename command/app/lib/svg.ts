export default class PanSVG {
	static svg: any
	static isPointerDown = false
	static containerSize = { width: 0, height: 0 }
	static rectSize = { width: 5_000, height: 5_000 }
	static pointerOrigin = { x: 0, y: 0 };
	static viewBox = { x: 0, y: 0 }
	static newViewBox = { x: 0, y: 0 }
	static scale = 9;
	private static pt: any
	static clickedPoint: { x: number, y: number }

	static getPointFromEvent(event: any) {
		return {
			x: event.clientX,
			y: event.clientY
		};
	}

	static onPointerDown(event: any) {
		PanSVG.isPointerDown = true;
		PanSVG.pointerOrigin = PanSVG.getPointFromEvent(event);
	}

	static onPointerUp(event: any) {
		PanSVG.isPointerDown = false;
		PanSVG.viewBox.x = PanSVG.newViewBox.x;
		PanSVG.viewBox.y = PanSVG.newViewBox.y;
	}

	static zoom(event: any) {
		PanSVG.scale += event.deltaY * -0.001;
		PanSVG.scale = Math.min(9, Math.max(3, PanSVG.scale));

		const viewBoxString = `${PanSVG.viewBox.x} ${PanSVG.viewBox.y} ${PanSVG.scale * PanSVG.containerSize.width} ${PanSVG.scale * PanSVG.containerSize.height}`;
		PanSVG.svg.setAttribute("viewBox", viewBoxString);
	}

	static click(event: any) {
		if (!PanSVG.pt) { PanSVG.pt = PanSVG.svg.createSVGPoint(); }
		const clientPt = PanSVG.getPointFromEvent(event);
		PanSVG.pt.x = clientPt.x;
		PanSVG.pt.y = clientPt.y;

		const cursorPt = PanSVG.pt.matrixTransform(PanSVG.svg.getScreenCTM().inverse());
		PanSVG.clickedPoint = {
			x: Math.round(cursorPt.x / 25), y: Math.round(cursorPt.y / 25)
		};
	}

	static onPointerMove(event: any) {
		if (!PanSVG.isPointerDown) return;
		event.preventDefault();

		const w = PanSVG.scale * PanSVG.containerSize.width;
		const h = PanSVG.scale * PanSVG.containerSize.height;

		const pointerPosition = PanSVG.getPointFromEvent(event);

		PanSVG.newViewBox.x = Math.max(0, Math.min(PanSVG.rectSize.width - w, PanSVG.viewBox.x - (pointerPosition.x - PanSVG.pointerOrigin.x)));
		PanSVG.newViewBox.y = Math.max(0, Math.min(PanSVG.rectSize.height - h, PanSVG.viewBox.y - (pointerPosition.y - PanSVG.pointerOrigin.y)));

		const viewBoxString = `${PanSVG.newViewBox.x} ${PanSVG.newViewBox.y} ${w} ${h}`;
		PanSVG.svg.setAttribute("viewBox", viewBoxString);
	}

	static addEventListeners() {
		if (!PanSVG.svg) { return; }
		PanSVG.svg.addEventListener("mousedown", PanSVG.onPointerDown); // Pressing the mouse
		PanSVG.svg.addEventListener("mouseup", PanSVG.onPointerUp); // Releasing the mouse
		PanSVG.svg.addEventListener("mouseleave", PanSVG.onPointerUp); // Mouse gets out of the SVG area
		PanSVG.svg.addEventListener("mousemove", PanSVG.onPointerMove);
		PanSVG.svg.addEventListener("wheel", PanSVG.zoom);
		PanSVG.svg.addEventListener("click", PanSVG.click);
	}

	static setViewBox() {
		if (!PanSVG.svg) { return; }
		const viewBoxString = `${PanSVG.viewBox.x} ${PanSVG.viewBox.y} ${PanSVG.scale * PanSVG.containerSize.width} ${PanSVG.scale * PanSVG.containerSize.height}`;
		PanSVG.svg.setAttribute("width", PanSVG.containerSize.width);
		PanSVG.svg.setAttribute("height", PanSVG.containerSize.height);
		PanSVG.svg.setAttribute("viewBox", viewBoxString);
	}
}