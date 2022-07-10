import { GetServerSideProps } from "next";

// eslint-disable-next-line @typescript-eslint/no-empty-function
export default function Redirect(): void { }

export const getServerSideProps: GetServerSideProps = async ({ res }) => {
	if (res) {
		res.writeHead(301, {
			Location: "/"
		});
		res.end();
	}
	return {
		props: {}
	};
};