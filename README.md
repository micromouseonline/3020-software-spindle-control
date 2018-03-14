
# 3020 SOFTWARE SPINDLE CONTROL


Thanks to [Johnny](https://hackaday.io/Johnny) from [hackaday.io](http://hackaday.io) for his [fantastic work](https://hackaday.io/project/6776-3040-cnc-milling-machine-mods/log/21618-jp-1482-spindle-controller-schematic) reverse-engineering the JP-1481 spindle board, seen below:

![JP-1482 Schematic](img/JP-1482_spindle_schematic.png)

### Problem

**You cannot control the spindle speed by any means other than the front panel knob.**

This is particularly important in an emergency stop situation.  While your controller will the stepper movement, the spindle will continue to spin.

The main interface (JP-382C) board routes pin 17 of the parallel port directly to the (unpopulated) "PWM OUT" connector on that board.  

The JP-1482 (spindle control) board has a PWM input, but the firmware on the STC15W408AS (the MCU that ultimately controls spindle speed) _does not_ respond to that input.  


### Solution

The solution is to use another MCU that reads from the existing front-panel knob, and the PWM from the interface board, that then simulates the front panel knob (using a programmable potentiometer).

The goal is to have a board you can simply plug in, and it _just works_.™

### Implementation

For the sake of ease, and getting up-and-running quickly, I opted to use an Arduino Pro Micro (5V), which I happened to have on hand.

The input PWM is buffered through a 6N136 opto isolator (since the PWM power supply is floating), and fed directly into the Arudino.

For output, there is an MCP41010 digital potentiometer, controlled via SPI. Since the front panel pot is 5K, we're only going to use ½ of it's 10K range.

Power (5v) is provided by the PWM board.

The software is standard Arduino code, though I'm using PlatformIO CLI since I'm not a big fan the Arduino IDE.  Why not bare metal AVR? This isn't a demanding application, and while the Arduino ecosystem isn't the most efficient, it does make it quick and easy to get things up-and-running.

### Construction

##### Schematic

![Schematic](img/schematic.png)

##### BOM

| Ref   | P/N       |
|-------|-----------|
| P1    | B2B-XH-A(LF)(SN)|
| P2, P4| B3B-XH-A(LF)(SN) |
| P3    | B8B-XH-A(LF)(SN) |
| R1    | Any 0802 330 ohm SMT resistor|
| R2    | Any 0802 2200 ohm SMT resistor |
| D1    | 1N4148 (optional)|
| U1    | 6N136, 8 DIP |
| U2    | Arduino Pro Micro (Ebay clone) |
| U3    | MCP41010, 8 DIP |

The connectors on the board are compatible with the connectors on the JP-1482; they're the JST XH series. Be careful buying random "XH" connectors from Ebay or even Amazon, for two reasons:

1. The connectors are often 2.54mm pitch, instead of 2.50mm (XH is 2.50mm).  This will mostly work, but for the 8-pin connector, you're going to bend the pins of your board connector by doing this.
1. Make sure the cables you use are the "straight through" type -- pin 1 on one end goes to pin 1 on the other end.

Buy genuine XH connectors and cables from a reputable supplier (e.g. Digi-key).

The board layout is rather tight.  It's designed to have U1 and U3 soldered directly to the board, while U2 is in a socket, for ease of swapping, above the other ICs.

#### Compiling and installing firmware from source

###### Step 1:
[Install Platformio](http://docs.platformio.org/en/latest/installation.html)

###### Step 2

You may need to add the `--upload-port` parameter, depending on how your system enumerates the Arduino port.

```bash
cd 3020-software-spindle-control/fw
platformio run -t upload
```

#### Installing from pre-compiled `.hex` file

--- nothing here yet ---

#### Behavior

The code will measure the input PWM from your controller, and monitors the front panel spindle control knob.  The front panel knob sets the _maximum_ speed.  If this knob is at zero, the spindle will not be enabled.  Example: if the knob is at the 60% position, and you command a 30% spindle speed via PWM, then the output speed will be 18% (0.3 x 0.6) of spindle absolute maximum speed. Also, the spindle on/off switch overrides everything else when in the off position.  
