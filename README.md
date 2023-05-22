# RadonMCA

RadonMCA is a basic Multichannel Analyzer (MCA) designed for a Radon Detection system. It is currently under development as part of a master's dissertation project.

The software provides essential functionalities, such as:
- Multiple consecutive acquisitions
- Start and stop functionality during acquisitions
- Threshold selection
- Range selection
- Basic peak analysis with two selection regions and linear background removal

## Prerequisites

To build this project, you need to have the following installed:
- Visual Studio
- Qt 6
- Qt Charts module
- Qt SerialBus and SerialPort modules

## Building the Project

The project is developed in Visual Studio and requires the installation of Qt 6, along with the modules related to Charts and SerialBus/SerialPort.

## Architecture

The software architecture consists of a `Device` class, which represents the Arduino used to communicate with the detection hardware. The `Device` class is part of the `Model` class, which runs in parallel with the `MainWindow`. The `Model` class is responsible for data acquisition, while the `MainWindow` handles all user interface elements and owns the timers used to track acquisition time.

## Contributing

If you have any suggestions, improvements, or bug reports, feel free to open an issue or submit a pull request on this repository. Your contributions are welcome!

