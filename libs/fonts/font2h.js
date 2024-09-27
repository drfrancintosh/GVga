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

let DEBUG = true;
function debug() {
    if (DEBUG) console.log(...arguments);
}
function die() {
    console.error(...arguments);
    process.exit(1);
}

let backslash = [0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00];
let circumflex = [0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00];
let underscore = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00];
let lbrace = [0x18, 0x30, 0x30, 0xc0, 0x30, 0x30, 0x18, 0x00];
let vertbar = [0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00];
let rbrace = [0x18, 0x0c, 0x0c, 0x03, 0x0c, 0x0c, 0x18, 0x00];
let tilde = [0x00, 0x76, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00];
let del = [0xaa, 0x54, 0xaa, 0x54, 0xaa, 0x54, 0xaa, 0x00];

async function main$(_opts) {
    let opts = _opts || gprocs.args("", "infile*,outfile*");
    // Read the binary file into a Buffer
    let buffer = fs.readFileSync(opts.infile);

    let outputFile = opts.outfile;
    fs.writeFileSync(outputFile, ""); // Clear the file if it exists

    // write the charmap to the output file in ascii order
    let hexString = "unsigned char fontData[] = {\n";
    for (let i = 0; i < 256; i++) {
        if (i == 0) {
            // entry 0x00 is blank
            for (let j = 0; j < 8; j++) {
                hexString += "0x00, ";
            }
            hexString += "\n";
        } else if (0 < i && i < 32) {
            // control characters are reverse of A-Z
            for (let j = 0; j < 8; j++) {
                let byte = (~buffer[i * 8 + j]) & 0xff;
                // Convert the byte to a hexadecimal string
                let hex = byte.toString(16).padStart(2, '0');
                // Append the hex string to the output
                hexString += `0x${hex}, `;
            }
            hexString += "\n";
        } else if (32 <= i && i < 64) {
            // numbers
            for (let j = 0; j < 8; j++) {
                let byte = buffer[(i - 32 + 32) * 8 + j];
                // Convert the byte to a hexadecimal string
                let hex = byte.toString(16).padStart(2, '0');
                // Append the hex string to the output
                hexString += `0x${hex}, `;
            }
            hexString += "\n";
        } else if (64 <= i && i < 96) {
            // uppercase letters
            for (let j = 0; j < 8; j++) {
                let byte = buffer[(i - 64) * 8 + j];
                if (i == 92) {
                    byte = backslash[j];
                } else if (i == 94) {
                    byte = circumflex[j];
                } else if (i == 95) {
                    byte = underscore[j];
                }
                // Convert the byte to a hexadecimal string
                let hex = byte.toString(16).padStart(2, '0');
                // Append the hex string to the output
                hexString += `0x${hex}, `;
            }
            hexString += "\n";
        } else if (96 <= i && i < 128) {
            // lowercase letters
            for (let j = 0; j < 8; j++) {
                let byte = buffer[(i - 96 + 128) * 8 + j];
                if (i == 123) {
                    byte = lbrace[j] >> 1;
                } else if (i == 124) {
                    byte = vertbar[j];
                } else if (i == 125) {
                    byte = rbrace[j] << 1;
                } else if (i == 126) {
                    byte = tilde[j];
                } else if (i == 127) {
                    byte = del[j];
                }

                // Convert the byte to a hexadecimal string
                let hex = byte.toString(16).padStart(2, '0');
                // Append the hex string to the output
                hexString += `0x${hex}, `;
            }
            hexString += "\n";
        } else if (128 <= i && i < 256) {
            // graphics characters... pass through
            for (let j = 0; j < 8; j++) {
                let byte = buffer[i * 8 + j];
                // Convert the byte to a hexadecimal string
                let hex = byte.toString(16).padStart(2, '0');
                // Append the hex string to the output
                hexString += `0x${hex}, `;
            }
            hexString += "\n";
        }
    }
    hexString += "};\n";

    // Write the hex string to the output file
    fs.writeFileSync(outputFile, hexString);
    debug(`Hex string written to ${outputFile}`);

}

module.exports = { main$ }

if (module.id === ".") {
    return main$();
}
