# Smart Door Lock – STM32 Project

This project implements a 4-digit smart door lock system using an STM32 microcontroller.
The system includes a matrix keypad, a 4-digit 7-segment display, two status LEDs, and a buzzer.
It verifies a preset password (1234) through a finite state machine (FSM) approach and provides real-time feedback to the user.

Hardware Overview:
- Microcontroller: STM32 (tested on NUCLEO-G474RE board)
- IDE: STM32CubeIDE
- Programming Language: C (HAL drivers)

Components:
- 4×4 Keypad – User input for password entry (0–9, *, #, A–D)
- 4-Digit 7-Segment – Displays entered digits
- LED_OK (Green) – Indicates correct password
- LED_ERR (Red) – Indicates incorrect password
- Buzzer – Audio feedback on wrong password

State Machine:
- ST_IDLE: Waits for * key to start password input.
- ST_COLLECT: Collects 4 digits entered by the user.
- ST_VERIFY: Compares entered digits to the stored password (1234).
- ST_SUCCESS: Lights green LED when the password is correct.
- ST_FAILURE: Activates red LED and buzzer for 2 seconds if wrong.

Features:
- Fully debounced matrix keypad scanning
- Non-blocking 7-segment multiplexing
- FSM-based architecture for deterministic behavior
- Brightness control via display refresh timing
- Configurable password in code
- Easy to port to other STM32 boards

Author:
Raymundo Gómez
Robotics and Digital Systems Engineer – Tecnológico de Monterrey
Currently in Korea for academic exchange
Passionate about embedded systems, robotics, and intelligent automation.

License:
Distributed under the MIT License.
