#!/usr/bin/env bun
import * as os from "os";

const bytes = await Bun.file("./dragon.bin").bytes();

let out = "const unsigned char DRAGON[] = {\n";
const bytesPerLine = 16;
for (let i = 0; i < bytes.length; i++) {
  // Insert an indent at the start of each line.
  if (i % bytesPerLine === 0) {
    out += "    ";
  }
  // Format each byte as 0x??
  out += "0x" + bytes[i].toString(16).padStart(2, "0");

  // Add a comma except for the final element.
  if (i !== bytes.length - 1) {
    out += ", ";
  }

  // Insert a newline after the designated number of bytes.
  if ((i + 1) % bytesPerLine === 0) {
    out += "\n";
  }
}
// Make sure we end with a newline if the last line wasn't complete.
if (bytes.length % bytesPerLine !== 0) {
  out += "\n";
}
out += "};\n";

const proc = Bun.spawn([`${os.platform() == "darwin" ? 'pbcopy' : 'wl-copy'}`], {
  stdin: "pipe"
});
proc.stdin.write(out);
proc.stdin.end();
await proc.exited;

console.log("C array copied to clipboard!");
console.log(out);
