ESP32 Handheld Arcade Console
A custom-built, portable gaming console featuring an ESP32-WROOM-32D microcontroller. Developed as a final project for Microprocessor Systems (EE-273) at UET, this device demonstrates real-time interaction through a modular Finite State Machine (FSM) software architecture.

🎥 Project Demo
Watch the console features and gameplay in action uploaded on youtube:
Project MPS - Handheld ESP32 Gamepad 
<<https://www.youtube.com/watch?v=V2o2-T2oCuM>>

🎮 Features & Applications
The console operates using a stable Clear-Draw-Display cycle to prevent screen flickering and offers several built-in applications:

Gaming Suite: Includes Snake (with Easy, Normal, and Hard modes), Flappy Bird, and Ping Pong against an AI opponent.

Utility Apps: A grid-based Calculator, a precise Stopwatch with start/stop/reset, and a Date & Time display.

Wireless Connectivity: Integrated ESP-NOW protocol allows the console to act as a remote controller for other ESP32 devices.

Persistence: High scores are saved to the ESP32’s flash memory via the EEPROM library, ensuring data persists after power loss.


💻 Software Architecture

FSM Logic: Manages transitions between menus and games seamlessly, ensuring inputs only affect the active application.

Input Debouncing: Software-level debouncing for tactile buttons (Home, Back, Action) to ensure clean signal processing.

Graphics: Utilizes the U8g2 library for monochrome rendering and coordinate-based drawing on the OLED.


🛠️ Hardware Specifications
The hardware utilizes a "sandwich structure" for durability and portability:

Main Processing: ESP32-WROOM-32D Dual-core processor.

Display: 1.3-inch OLED (SH1106) using I2C protocol (SDA: GPIO 21, SCL: GPIO 22).

Input: Analog Joystick (VRx: GPIO 34, VRy: GPIO 35) and three tactile push buttons.

Power Subsystem: Dual 1800mAh Li-ion batteries with an AMS1117-3.3V regulator and a TP4056 USB-C charging module.

Chassis: Rigid Vero board circuit layer protected by laser-cut acrylic faceplates and metallic spacers.