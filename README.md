# Real Time Vehicle Monitoring System
Design, implement, test, and analyze of a simplified version of a real time system for monitoring of vehicle’s health conditions in C++ run on QNX Neutrino Real-Time Operating System

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

