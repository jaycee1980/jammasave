# cch-jammasave
A CRT power saver for JAMMA arcade cabinets.

At the museum we have a few arcade cabinets for visitors to play on. These traditionally use CRT monitors -  CRT's are a problem, because they wear out with use, and draw quite a bit of power. The machines stand idle often, so the CRT displays are needlessly on, being worn down and costing extra money to run. As CRT's are no longer being made, it can be problematic to replace them when they fail.

PC's solved this issue by having screensavers that power off the monitor when the machine is idle. However, arcade machines typically run an "attract" mode to entice people to play on them, thus there is no such setting.

So what do we do ? Solve it with extra hardware of course :)

## How it works

The board is designed to be wired to 5V power (easily obtainable in an arcade cabinet), and has a relay which is normally switched on, but switches off when a timeout period expires. There are  16 "wire-OR'd" inputs which are connected to the JAMMA connectors input pins (See https://wiki.arcadeotaku.com/w/JAMMA for more info). When any of these 16 inputs are pulled low (i.e. the user presses a button or pushes a direction), the timer is reset. The CRT's power feed can be wired through this relay so that it is switched off when the timer expires.

A preset potentiometer allows the timeout period to be adjusted from 30 minutes to 1½ hours. This can be further adjusted in the software if required.
A jumper can also put the board into "disable" mode, where the relay is switched on and nothing else happens. This is useful for servicing.
