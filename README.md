# Serial-CNC-Stepper

This project showcases the core aspects of a CNC machine and how to control it.

A CNC (Computer Numeric Control) machine is a type of machine that can move according to absolute or incremental coordinates, and perform certain actions on the given location, or on-route between two sets of coordinates. Examples include CNC routers, 3D printers, laser cutters etc. 
The coordinate instructions are typically stored in a file called a gcode (file extensions can vary, examples are .nc, .gcode).
The main control logic is in charge of converting coordinates to machine instructions. 
In simple CNC systems, the main control logic may be software-based, and the hardware-side of the CNC machine is limited in the sense that it does not have a buffer to store the whole gcode. Those systems therefore rely on external devices, which can store, translate, and communicate the content of the gcode to the logical CNC controller. 
In many cases, the gcode is stored on a PC. The gcode is loaded through a type of software called a trajectory planner. The trajectory planner has information about the machine's physical setup, steps/unit, acceleration curves, input/output devices, etc. It also keeps track of current axes locations and I/O states. 
The motors are typically stepper-motors, which move one step at a time in the desired rotary direction. Stepper motors have a motor-core (rotor) with magnetized teeth. Opposite the rotor, there are magnetic coils fixed to the motor case. When current flows through these coils, they will attract the nearest magnetized teeth of the rotor.

In the case of simple CNC systems, the motion-instructions are communicated through a parallel port (such as the old printer port) in real time. Typically, there will be three pins per axis: Step, Direction, Enable. These pins are switched according to the desired motion. 
For example: For the X-axis, the enable pin can be set to positive, and then 200 steps can be sent out on the step pin over the course of one second. This means that a typical stepper motor will move one whole rotation in one second. 
In extended setups, the stepper motor resolution can also be controlled by additional pins, in order to subdivide each step by a certain factor. In practice, this means that the coils in the motor will maintain a proportional current load for each step in order to hold the motor-core fixed in magnetic middle-stages.
Step, direction, enable, (and resolution), logical values are sent to a motion controller. This is typically an integrated circuit, which creates high-low patterns to match how the motor coils must be switched respectively in order to perform a step. 
Between the logical motion controller and the motor itself, is a motor current driver, which is a simple transistor (for example a MOSFET), that lets current flow from a power source and through the motor coils. 

To sum up, in these simple systems, step instructions must be communicated parallelly in real time from the PC, to match the relative motion of all axes, at the designated velocity.
There are some major drawbacks with this method: 
* Parallel communication on PCs is done through the printer-port. This port is of course old and not available on most modern PCs and laptops. 
* The parallel printer port is also limited by its bandwidth. On higher resolution machines, many steps must be executed second, and the tick rate of the port may not be high enough to hold all the required steps. 
* The parallel port is highly susceptible to noise, and by default, there is no acknowledgement of sent/received information. The logical state of pins in the parallel port is fragile. Therefore, noise can induce unwanted steps, or cause steps to be lost. 
* When stepping at higher speeds, the logical readout is being smoothed. This along with the noise factor, can blur out logical states and render them unreadable by the receiving motion controller.
In short, there's a lot of room for error when using the parallel port step method for CNC systems.

This project showcases a CNC system with an alternative communication form between the PC and the motion controller. Specifically, the printer-port is avoided, and instead serial communication is used between the PC and the control logic. Instead of PC trajectory planning software, the conversion from gcode to (trajectory planning) steps/direction, is performed by an MCU (Micro Controller Unit).
The method used in this project diverges in the following:
* The goal of the PC is simply to store and communicate the gcode
* gcode is communicated one line at a time
* A custom motion controller is coded to perform the following tasks:
    * Receive gcode serially, one line at a time
    * Perform trajectory planning directly in the MCU


The project covers the following points:
* User control interface 
* Gcode serialization
* Communication between PC and receiving controller
* Motion control logic
* Showcase of a working CNC system
