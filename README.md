# Real Time Vehicle Monitoring System
Design, implement, test, and analyze of a simplified version of a real time system for monitoring of vehicle’s health conditions in C++ run on QNX Neutrino Real-Time Operating System

### Project description:
Implementation of a set of real time tasks that will periodically obtain the vehicle’s data and display it to the driver
- Data provided every 1 second collected from different sensors
- Data production process is performed by a set of periodic tasks
- Each periodic task will read from CSV dataset
- Use IPC techniques to provide the read value to the consumer real-time time (display obtained value to user)
- Timers and clock interrupts must be used to implement periodic tasks
- Periodic task may only execute once per period at default period of 5s

Data producer: Read given data stored in dataset for each periodic task
-> One periodic task for each variable
	- Fuel Consumption,
	- Engine Speed (RPM),
	- Engine Coolant Temperature,
	- Current Gear,
	- Vehicle Speed

Data consumer: Display information to driver by periodically communicating with data producer using IPC
-> One periodic task for each value to display

Dataset provides measurements for every 1 second.
- Data must be preprocessed
- Create separate files for each variable of interest (Preprocess data)
- Read data in hops of 5s

Periodically collect data from vehicle’s onboard sensors and provide different real-time services for the driver and passengers

### Program setup instructions:
Firsly, it is expected the user has a valid and appropriate license for QNX version 7.0+. Open a Momentics IDE session.

1. Create a new launch target
	- Select `QNX Virtual Machine Target`
	- VM platform `VMWare` or `VBox`
	- CPU Arch `x867_64`
	- IP Address:
		- Search `Network Connections` on Windows
		- Under the selected VM Platform network adapter, navigate to IPV4 properties. The IP present will the the one entered into QNX
		- It is important to modify the last digit, the port number, to be different from the one in the IPV4 window. The range is 0 to 255.

2. Allow the VM to boot
	- Enter `ipconfig`
	- If there are issues with the VM target connecting on QNX, create a new launch target `QNX Target` with the IP provided by the previous command

3. Creating new project
	- Select New Project -> `QNX Project`
	- `C/C++` and `QNX Executable`
	- Enter a project name `main` and select language `C` and CPU Arch `x86_64`
	- Replace the `main.c` file created with the one included with the project

4. Running the project
	- Click the run button
	- The console should prompt the user with the `Select program setup options` menu
	- The user has the ability to select different operating modes throughout the execution of the program
	- These options include running in the default mode, where each task runs with a period of 5 seconds, to manually select all periods, to edit a specific period, to continue execution of the program as it was initialized previously, or to exit the program.
	- Enter a value from 0 to 4 for the selection
		- Selection 0: Watch as the program executes, producing data for each VOI every 5 seconds, after 20 seconds (4 major cycles), the user will be prompted with options again
		- Selection 1: The user will be prompted to enter a period, in seconds, for each producer, the program will then execute for the major cycle of those periods, and produce data at the correct times. After the execution the user will be prompted with options again
		- Selection 2: The use will be prompted to select a specific producer, from 0 to 4, to modify the period, enter the number of the producer you wish to modify then, when prompted, enter the value in seconds to set for that producer. The user will be prompted with options again after the execution
		- Selection 3: This will continue the execution with the parameters previously set, or with the default if it is the first execution.
		- Selection 4: The program will exit

	- __Note: It is important to only enter valid integer numbers during the prompts, C does not have input validation and entering a value of 2.0 instead of 2 will cause serious execution issues. Due to the input buffer and that C uses IEEE 754 for float values, the value 2.0 will not be converted to 2.__ 
