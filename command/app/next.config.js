/* eslint-disable @typescript-eslint/no-var-requires */
/* eslint-disable no-undef */
const withTM = require("next-transpile-modules")(["lodash-es", "react-d3-speedometer"]);
module.exports = withTM({
	future: {
		webpack5: true
	}
});