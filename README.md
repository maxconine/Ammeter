# Arduino INA226 USB Ammeter

This project measures DC current using an Arduino Leonardo, an INA226 current-sense module, and an external high-current shunt. Measurements are sent to a computer over USB and displayed in a browser-based dashboard.

The dashboard shows the current in amps, samples the sensor every 100 ms, and plots current over time. Measurement can be started, stopped, cleared, and zeroed directly from the browser.

## Features

* Measures approximately 5–30 A DC
* Uses an external 100 A / 75 mV shunt
* Samples current every 100 ms
* Displays live current in amps
* Plots current versus elapsed time
* Start and stop controls
* Zero-offset calibration
* USB communication through the browser
* Stores up to 6,000 plotted samples, equivalent to 10 minutes
* No separate desktop application required

## Hardware

* Arduino Leonardo
* INA226 current-sense module
* External current shunt
* USB cable
* DC load and power source
* Thin shunt sense wires

The default firmware is configured for a:

```text
100 A / 75 mV shunt
```

This shunt has a resistance of:

```text
0.00075 ohm
```

The high load current flows through the external shunt, not through the Arduino or INA226 module.

## Wiring

### Arduino Leonardo to INA226

| Arduino Leonardo | INA226 |
| ---------------- | ------ |
| 5V               | VCC    |
| GND              | GND    |
| SDA / Pin 2      | SDA    |
| SCL / Pin 3      | SCL    |

### External shunt

```text
Battery negative ── External shunt ── Load negative
                         │       │
                         │       └── INA226 VIN+
                         └────────── INA226 VIN-
```

If the reported current is negative, swap `VIN+` and `VIN-` or change the current direction setting in the firmware.

Do not route the 5–30 A load through the small onboard shunt found on many INA226 modules. The INA226 must measure the voltage across the large external shunt.

## Repository Files

```text
ina226_leonardo.ino
current_monitor.html
```

### `ina226_leonardo.ino`

Arduino firmware that:

* Reads the INA226
* Calculates current from the external shunt voltage
* Samples every 100 ms
* Accepts commands from the browser
* Sends measurements over USB serial

### `current_monitor.html`

Browser dashboard that:

* Connects to the Arduino through Web Serial
* Displays current in amps
* Starts and stops measurements
* Performs zero calibration
* Plots current over time
* Clears recorded data

## Arduino Setup

Install the following library using the Arduino IDE Library Manager:

```text
INA226 by Rob Tillaart
```

Open `ina226_leonardo.ino`, select the Arduino Leonardo board and serial port, and upload the sketch.

The default shunt configuration is:

```cpp
constexpr float SHUNT_RATED_CURRENT_A = 100.0f;
constexpr float SHUNT_RATED_VOLTAGE_V = 0.075f;
```

Change these values if a different shunt is used.

If the current direction is reversed, change:

```cpp
constexpr float CURRENT_DIRECTION = 1.0f;
```

to:

```cpp
constexpr float CURRENT_DIRECTION = -1.0f;
```

## Running the Web Dashboard

The dashboard uses the Web Serial API and should be opened through a local web server.

From the project directory, run:

```bash
python3 -m http.server 8000
```

Then open the following address in desktop Chrome or Microsoft Edge:

```text
http://localhost:8000/current_monitor.html
```

Web Serial is not supported by every browser. Desktop Chrome or Edge is recommended.

Close the Arduino Serial Monitor before connecting through the dashboard because only one program can normally access the serial port at a time.

## Usage

1. Connect the Arduino Leonardo to the computer over USB.
2. Open the dashboard in Chrome or Edge.
3. Click **Connect Arduino**.
4. Select the Leonardo serial port.
5. Make sure no current is flowing.
6. Click **Zero sensor**.
7. Click **Start measurement**.
8. Apply the load and observe the current plot.
9. Click **Stop** to end the measurement.
10. Click **Clear plot** to remove recorded samples.
11. Click **Export CSV** to download a CSV file containing data.

Do not zero the sensor while current is flowing. Zero calibration treats the present reading as 0 A.

## Serial Protocol

The browser sends the following commands:

```text
START
STOP
ZERO
PING
```

The Arduino sends measurements in this format:

```text
DATA,elapsedMilliseconds,currentAmps
```

Example:

```text
DATA,2300,12.4831
```

This represents:

```text
Elapsed time: 2300 ms
Current: 12.4831 A
```

Status messages use the following format:

```text
STATUS,READY
STATUS,MEASURING
STATUS,STOPPED
STATUS,ZEROING
```

## Current Calculation

Current is calculated from the measured shunt voltage:

```text
Current = Shunt voltage / Shunt resistance
```

For a 100 A / 75 mV shunt:

| Current | Shunt voltage |
| ------: | ------------: |
|     5 A |       3.75 mV |
|    10 A |       7.50 mV |
|    20 A |      15.00 mV |
|    30 A |      22.50 mV |

## Safety

* Do not connect the load current through the Arduino.
* Do not connect the load current through the small INA226 breakout-board traces.
* Use properly rated wire, terminals, connectors, and fusing.
* Place the shunt in the low-side return path unless the complete system grounding arrangement has been reviewed.
* Be careful when connecting a USB-grounded computer to an externally powered circuit.
* Confirm the INA226 module wiring and voltage limits before applying power.
* Keep the high-current connections separate from the thin shunt sense wires.

## Limitations

* The browser must remain open during measurement.
* Data is stored in browser memory and is not automatically saved.
* The default plot retains approximately 10 minutes of measurements.
* Accuracy depends on the shunt tolerance, wiring resistance, INA226 configuration, and zero calibration.
* The current dashboard does not yet export CSV data.

## Possible Improvements

* Adjustable sample interval
* Automatic scaling and fixed current ranges
* Measurement session naming
* Long-term data logging
* Energy and amp-hour calculations
* Calibration against a reference meter


Parts Used:
- [Digital Ammeter]([url](https://www.amazon.com/CGELE-Multifunction-Battery-Multimeter-Voltmeter/dp/B08Y61CNLK?sr=8-1))
- [Precision Voltage sensor module]([url](https://www.amazon.com/HiLetgo-Bi-Directional-Current-Monitoring-Function/dp/B0CDWXB8PG?sr=8-3))
- [Arduino dev boards]([url](https://www.amazon.com/dp/B0D83FBYPD))

Interface:

<img width="707" height="589" alt="Screenshot 2026-07-23 at 4 29 03 PM" src="https://github.com/user-attachments/assets/cbc8104b-e948-4ecc-a5d5-f663fb4fc836" />
