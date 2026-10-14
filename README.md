# NanoPi: identical firmware on two Nanos

Flash the same firmware to both ATmega328P Nanos. One connects to the Pi's
existing I2C bus at address 0x04; the other connects to the ultrasonic sensor.
Either board can occupy either position without a firmware role setting.
This supports exactly two Nanos on a point-to-point MAX485 link, with one
request initiator at a time. It is not a multi-drop or multi-hop protocol.

## Wiring (the same on both Nanos)

| Nano | Ordinary 5 V MAX485 module |
| --- | --- |
| D2 | RO (receive output) |
| D3 | DI (transmit input) |
| D4 | DE and /RE tied together |
| Regulated 5 V | VCC |
| GND | GND |

Connect A to A and B to B over one outdoor-rated twisted pair. Follow module
documentation if its A/B labels use a different convention. Provide the bus
reference connection per the transceiver wiring requirements. For a 120-ohm
cable, terminate A/B with 120 ohms at each physical end (check for onboard
termination first), and provide fail-safe biasing at one location. A bare
MAX485 does not guarantee a defined idle state on a terminated, undriven bus.
Use a pull-down (for example 10 kohms) on DE+/RE so each driver stays disabled
during reset. Choose suitable outdoor surge protection and isolation.

On the dam Nano, connect sensor Trig to D9 and Echo to D10, with the sensor's
specified power supply and common local ground. Confirm the exact sensor
model's voltage, range and interface before wiring.

D0/D1 stay assigned to USB Serial at 9600 baud. D2/D3/D4 are reserved for
MAX485, and A4/A5 (18/19) for I2C, on both boards. These pins cannot be used by
Pin or US commands. A6/A7 are analog inputs only: reading is allowed, writing
or using them for ultrasonic Trig/Echo is rejected.

SoftwareSerial runs at 9600 baud so USB debugging remains available. It masks
interrupts during character reception/transmission; CRC checking catches
corrupted frames, but cannot guarantee delivery under every I2C/USB load.
Bench-test simultaneous I2C polling and RS485 traffic before deployment.
If that load is unreliable, use a controller with a second hardware UART or
an external UART, rather than assuming software serial is equivalent to one.

## Pin routing

Local pins use 0-21. Remote pins use 22-43: add 22 to the physical pin number.
The reserved-pin restrictions above apply to both banks.

| Operation | USB text command | Binary command (hex) |
| --- | --- | --- |
| Local ultrasonic D9/D10 | `US,9,10` | `AA 02 09 0A` |
| Remote ultrasonic D9/D10 | `US,31,32` | `AA 02 1F 20` |
| Local analog A0 read | `Pin,Read,14` | `AA 01 01 0E` |
| Remote analog A0 read | `Pin,Read,36` | `AA 01 01 24` |
| Remote D5 relay on | `Pin,Write,27,On` | `AA 01 00 1B 01` |

Relay outputs retain the existing active-low behavior. Use numeric pin values
in these commands. Both ultrasonic pins must address the same board and must
be different digital-capable pins. Out-of-range and mixed-board requests fail
validation. Routing subtracts 22 from pin fields only, not from other arguments.
A request received over RS485 may execute locally but cannot be forwarded again.

The Pi's I2C framing and response format are unchanged. Change the configured
ultrasonic pair from `(9, 10)` to `(31, 32)` to select the remote sensor, e.g.
`nano.ultrasonic_read_multiple([(31, 32)])` with the included Python client.
The client already accepts numeric pins without a local-only range limit.

RS485 packets have a length, request/response kind, 16-bit sequence ID and
CRC16. Missing or corrupt replies time out after 220 ms and return status
`0xF9` (REMOTE_TIMEOUT), rather than a stale distance. Sensor no-echo remains
`0xFB`. I2C still reports `0xFA` while work is pending. A seven-command batch
with missing replies can take about 1.7 seconds plus debug output; the included
client has a two-second polling window. Keep debug off for normal operation.
Other Pi clients should allow for the added link latency and handle nonzero
status values as errors. The supplied Python client already rejects unknown
nonzero status codes; naming 0xF9 in it is optional, not a framing change.

The link does not automatically retry output commands: a timeout can mean
the operation executed but its acknowledgement was lost. Existing Pi-side
retries still apply. Avoid simultaneous remote commands from both USB consoles
or from USB and the Pi. USB text/debugging remains available on either board;
commands arriving over RS485 are processed without verbose device traces.
Send USB commands terminated with a newline. Partial lines do not block service.

## Power over 40 metres

Use a separately fused 12 V or 24 V DC supply and appropriately sized power
conductors, with a buck converter at the dam providing regulated 5 V to the
Nano, MAX485 and compatible sensor. Never connect 24 V directly to the Nano.
Do not rely on a 40 m thin-wire 5 V feed. Check the board's external-power/USB
arrangement before connecting USB while powered externally, and put the remote
electronics in a weatherproof enclosure.

## Build and check

Build: `pio run -e nanoatmega328-old` (or `nanoatmega328` for the newer bootloader).
Upload each board separately using the appropriate environment and port.
The firmware is identical; upload baud depends on the board's bootloader.
No upload is performed by the build command.

Compile the Unity tests without uploading or running them:
`pio test -e nanoatmega328-old-test --without-uploading --without-testing`.
Run them on a spare Nano with `pio test -e nanoatmega328-old-test`, then reflash
the production firmware. Tests cover routing, offset boundaries, reserved pins,
mixed-board rejection, link errors, CRC parsing and existing registry/watchdog
behavior. They do not verify electrical timing or cable performance.

Hardware acceptance checklist:

1. Start with a short cable. Confirm local analog/relay commands still work.
2. Confirm `US,31,32` measures the remote sensor in centimetres through USB and
   through the Pi; `US,9,10` still addresses the local board.
3. Disconnect RS485 and confirm 0xF9; disconnect the sensor and confirm 0xFB.
4. Reconnect and confirm measurements recover without a reboot.
5. Swap the two flashed Nanos, keeping each location's wiring, and repeat.
6. Test a mixed batch of local pressure reads and remote ultrasonic requests,
   including sustained I2C polling and USB debug output. Then repeat over 40 m.
7. Check the regulated supply at the remote board under load, and validate
   the sensor's distance range against the dam's highest and lowest levels.
