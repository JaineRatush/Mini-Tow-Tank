# Mini-Tow-Tank
Developed a mini tow tank to support research on bioluminescent algae as an alternative flow visualization technique. The control system uses closed-loop encoder feedback to achieve precise velocity and repeatable motion, creating the flow conditions required for observing Kármán vortices in algae.

Design Notes & Instructions: https://drive.google.com/file/d/1_0wM9CwElLxKBJwhXemWCHgTlYO9H7mf/view?usp=sharing

## Mechanical
- First principles to choose the correct motor based on electrical constraints, gantry load, and target velocity
- Machined custom parts{4mm to 5mm motor shaft adapter, Electronics holder, cut 1010 extrusions to length}
- Used CAD to design {Belt tensioner and Gantry subassemblies} using a 3D Printer and Laser Cutter

## Controls
Created a script to calibrate Kff(feed-forward PWM command for desired speed)

Created a script that:
- commands a precise velocity(+/- 0.03 m/s from target velocity) using closed loop feedback from a magnetic relative encoder and coded PI control.
- Uses Feedforward to set a base PWM for the motor
- Capable of trapezoidal motion(not currently being used but commented out)
- Stops the gantry at a set location

## Electrical
- Learned about the different types of Encoders(Relative/Absolute, Magnetic/Rotary/Optic)
- Learned about Single H-Bridges & MOSFETs
- Learned about Low-Pass Filters 
- Wired & Created a Wiring diagram for the electronics:
 {Arduino, Proximity Sensor, IBT2 Motor Driver, Motor, Magnetic Encoder}

Link to Video: https://www.youtube.com/watch?v=F2ftbyinyw4
