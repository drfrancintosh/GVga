#!/usr/bin/env node

/** 
 * template for a main.js file
 * C64 Fonts: https://www.zimmers.net/anonftp/pub/cbm/firmware/characters/
 **/

const glstools = require('glstools');
const gprocs = glstools.procs;
const gstrings = glstools.strings;
const gfiles = glstools.files;
const fs = require("fs");
const path = require("path");

let DEBUG=true;
function debug() {
    if (DEBUG) console.log(...arguments);
}
function die() {
    console.error(...arguments);
    process.exit(1);
}

async function main$(_opts) {
    let opts = _opts || gprocs.args("", "infile*,outfile*");
    // Read the binary file into a Buffer
    let buffer = fs.readFileSync(opts.infile);

    let outputFile = opts.outfile;
    fs.writeFileSync(outputFile, ""); // Clear the file if it exists

    let hexString = "unsigned char fontData[] = {\n";
    for (let i = 0; i < buffer.length; i++) {
        let byte = buffer[i];
        // Convert the byte to a hexadecimal string
        let hex = byte.toString(16).padStart(2, '0');
        // Append the hex string to the output
        hexString += `0x${hex}`;
        // Add a comma after each byte except the last one
        if (i < buffer.length - 1) {
            hexString += ", ";
        }
        // Add a newline after every 16 bytes
        if ((i + 1) % 16 === 0) {
            hexString += "\n";
        }
    }
    hexString += "};\n";

    // Write the hex string to the output file
    fs.writeFileSync(outputFile, hexString);
    debug(`Hex string written to ${outputFile}`);
    
}

module.exports = {main$}

if (module.id === ".") {
    return main$();
}
