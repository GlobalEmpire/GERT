'use strict';

const fs = require("fs");
const rl = require("readline");
const io = rl.createInterface({
	input: process.stdin,
	output: process.stdout,
	prompt: "ADDRESS KEY: "
});

var fd = fs.openSync("./resolutions.geds", 'a');

function byteify(str, buffer) {
	let segs = str.split(".");
	let high = parseInt(segs[0], 10);
	let low = parseInt(segs[1], 10);
	
	buffer[0] = high >> 4;
	buffer[1] = high << 4 & 0xF0 | low >> 8;
	buffer[2] = low & 0xFF;
}

function readLine(line) {
	if (line[0] == "q") {
		fs.closeSync(fd);
		process.exit(0);
	}
	
	let inputs = line.split(" ");
	
	let output = Buffer.alloc(23);
	
	let addr = byteify(inputs[0], output);
	let key = inputs[1];
	
	output.write(key, 3);
	
	fs.writeSync(fd, output);
	
	io.prompt();
}

io.on("line", readLine);
io.prompt();