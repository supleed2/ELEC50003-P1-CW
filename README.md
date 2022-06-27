# Electrical Engineering Y2: Mars Rover Project

The goal is to design + build an autonomous rover that could be used remotely without supervision. There is a processing using (ESP32) that can accept a queue of movement or other commands and return the current status data to the control interface when the rover has signal. The rover can also detect and avoid collisions while routing to reach the target location. A charging station, outside the scope of this project, would be used to recharge the rover as an alternative to the in-built solar charging. Base project files can be found [here](https://github.com/edstott/EEE2Rover).

## Submodules

### [Vision](./Vision/)

Quartus project containing files to connect to the FPGA camera and stream video back to the Control module to be processed and passed to the Command Dashboard. Initial bound detection of colours and shapes is done on the FPGA fabric.

### [Control](./Control/)

PlatformIO project containing the Control logic to host the Command Dashboard, as well as forward commands to relevant modules and return information to the user dashboard.

### [Command](./Command/)

Dashboard for viewing the recorded map data and status of the Rover as well as sending new commands, hosted directly by the Control module ESP32 during testing.

### [Drive](./Drive/)

Code for the drive module which accepts movement commands indicating distance or rotation, and returns the current status / position, calculated using existing commands if complete, or the estimated posiiton if incomplete. Uses a feedback loop to achieve the requested motion.

### [Energy](./Energy/)

Code for the energy module which handles charging as well as battery health monitoring and maintenance. 

### [IMU](./IMU/)

An extra module used to provide more reliable direction data due to the drift of the direction recorded by the drive module.
