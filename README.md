# jammasave
A CRT power saver for JAMMA arcade cabinets.

![Picture of the board](/media/Assembled\%20Board.jpg)

At the [Centre for Computing History](http://www.computinghistory.org.uk) we have a few arcade cabinets for visitors to play on. These traditionally use CRT monitors -  CRT's are a problem, because they wear out with use, and draw quite a bit of power. The machines stand idle often, so the CRT displays are needlessly on, being worn down and costing extra money to run. As CRT's are no longer being made, it can be problematic to replace them when they fail.

PC's solved this issue by having screensavers that power off the monitor when the machine is idle. However, arcade machines typically run an "attract" mode to entice people to play on them, thus there is no such setting.

So what do we do ? Solve it with extra hardware of course :)

## How it works

The board is designed to be wired to 5V power (easily obtainable in an arcade cabinet), and has a relay which is normally switched on, but switches off when a timeout period expires. There are  16 "wire-OR'd" inputs which are connected to the JAMMA connectors input pins (See https://wiki.arcadeotaku.com/w/JAMMA for more info). When any of these 16 inputs are pulled low (i.e. the user presses a button or pushes a direction), the timer is reset. The CRT's power feed can be wired through this relay so that it is switched off when the timer expires.

A preset potentiometer allows the timeout period to be adjusted from 30 minutes to 1½ hours. This can be further adjusted in the software if required.
A jumper can also put the board into "disable" mode, where the relay is switched on and nothing else happens. This is useful for servicing.

# Details

The design is based around an [Atmel ATtiny13 microcontroller](https://www.microchip.com/wwwproducts/en/ATtiny13). An internal 4.8MHz oscillator means there is no need for an external timing crystal.

The internal ADC allows a simple preset potentiometer, wired as a potential divider, to be used in order to set the timeout delay. The ADC simply reads a value between 0-1023 which is then used to set the initial value of the timeout counter.

One GPIO port is configured as an output, and used to control a a "sugar cube" type relay, via a BC337 transistor. The relay is a 5V type so that only a 5V supply is required. This relay then controls the CRT by switching it's power feed.

JAMMA inputs are active low, so this means that multiple inputs can be catered for simply by using diodes. A GPIO pin is configured as an input, which is normally pulled up th +5V via a 100K resistor. A path to ground through any one of the diodes pulls the input low, which is detected by using pin change interrupts.

The timeout period is implemented using Timer/Counter 0. This is configured to generate ~500 ticks/second. This is used to decrement a tiemout counter which is initially set according to the timeout period required. Once this counter reaches zero, the relay is switched off. If the pin change interrupt fires due to an input, the timeout counter is reset.

# Whats in this repo

This repo contains the EAGLE design files, Gerber files suitable for manufacturing, the C source code and Atmel Studio project files used to compile the code.
