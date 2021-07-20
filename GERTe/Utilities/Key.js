'use strict';

const fs = require("fs");
const rl = require("readline");
const io = rl.createInterface({
	input: process.stdin,
	output: process.stdout,
	prompt: ""
});

const crypto = require("crypto");

const addrPattern = /^(?:(\d{1,4}\.\d{1,4}):)?(\d{1,4}\.\d{1,4})$/;
const numberPattern = /\d{1,4}/g;

var fd = fs.openSync("./resolutions.geds", 'a');

/**
 * Tests compliance of an arbitrary string containing an address.
 * <br>
 * <br>
 * Note: Modified to reject GERTc addresses
 *
 * @param {string} address Address to test compliance of.
 * @returns {boolean}
 */
function compliant(address) {
	const matches = address.match(addrPattern);
	if (matches == null || matches[3])
		return false;

	for (const num of address.matchAll(numberPattern)) {
		const numval = parseInt(num);
		if (numval > 4095 || numval < 0)
			return false;
	}

	return true;
}

function byteify(str) {
	let segs = str.split(".");
	let high = parseInt(segs[0], 10);
	let low = parseInt(segs[1], 10);
	let buffer = Buffer.alloc(3);
	
	buffer[0] = high >> 4;
	buffer[1] = high << 4 & 0xF0 | low >> 8;
	buffer[2] = low & 0xFF;
	
	return buffer;
}

function onAddress(addr) {
	addr = addr.trim();

	if (addr.toLowerCase() == "q") {
		fs.closeSync(fd);
		process.exit(0);
	}

	if (!compliant(addr)) {
		console.error("Address is not GERTe compliant.");
		io.question("Address: ", onAddress);
		return;
	}

	io.question("Key path: ", (path) => {
		path = path.trim()

		if (path.toLowerCase() == "q") {
			fs.closeSync(fd);
			process.exit(0);
		}

		let key;
		try {
			key = fs.readFileSync(path);
		} catch(e) {
			console.error("Path not found!");
			onAddress(addr);
			return;
		}

		let test = crypto.createPublicKey({key: key, format: "der", type: "spki"});
		if (test.type != "public" || test.asymmetricKeyType != "ec") {
			console.error("Key is invalid");
			onAddress(addr);
			return;
		}
		
		let len = Buffer.alloc(2);
		len.writeUInt16BE(key.length);

		fs.writeSync(fd, byteify(addr));
		fs.writeSync(fd, len);
		fs.writeSync(fd, key);

		io.question("Address: ", onAddress);
	});
}

console.log("Enter Q at any time to quit.");
io.question("Address: ", onAddress);
