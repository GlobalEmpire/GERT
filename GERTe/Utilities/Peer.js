'use strict';

const fs = require("fs");
const rl = require("readline");
const io = rl.createInterface({
	input: process.stdin,
	output: process.stdout,
	prompt: "IP PORT PORT: "
});

function ip2int(ip) { // Ripped from jppommet on GitHub. Thanks!
    return ip.split('.').reduce(function(ipInt, octet) { return (ipInt<<8) + parseInt(octet, 10)}, 0) >>> 0;
}

var fd = fs.openSync("./peers.geds", 'a');

function readLine(line) {
	if (line[0] == "q") {
		fs.closeSync(fd);
		process.exit(0);
	}
	
	let inputs = line.split(" ");
	
	let ip = ip2int(inputs[0]);
	let inPort = parseInt(inputs[1], 10);
	let outPort = parseInt(inputs[2], 10);
	
	let output = Buffer.alloc(8);
	output.writeUInt32BE(ip, 0);
	output.writeUInt16BE(inPort, 4);
	output.writeUInt16BE(outPort, 6);
	
	fs.writeSync(fd, output);
	
	io.prompt();
}

io.on("line", readLine);
io.prompt();