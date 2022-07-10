import { MouseEventHandler } from "react";

interface CommandButtonProps {
	text: string,
	padding?: number,
	darkMode: boolean
	onClick: MouseEventHandler
}
export default function CommandButton({ text, onClick, padding, darkMode }: CommandButtonProps): JSX.Element {
	return <button
		className={`${darkMode ? "text-white" : "text-black"} m-2 p-${padding ? padding : 3} text-2xl ${darkMode ? "outline-white" : "outline-black"} out`}
		onClick={onClick}
	>
		{text}
	</button>;
}