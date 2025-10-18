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

---

## 🖥️ How to Build and Flash

1. Open the project in **STM32CubeIDE**
2. Connect your Nucleo board via USB
3. Click **Build → Build Project**
4. Click **Run → Debug or Run**
5. Observe real-time operation on your hardware

---

## 🧠 How It Works

1. Press `*` → system starts listening for password input.  
2. Enter 4 digits → each shown on one of the four 7-segment displays.  
3. Press `*` again to verify.  
   - ✅ Correct → green LED turns on.  
   - ❌ Incorrect → red LED + buzzer for 2 seconds.  
4. You can restart anytime by pressing `#` or `*`.

---

## 🔬 Technical Notes

- Display multiplexing handled manually in software loop.
- Each digit controlled individually via GPIO pins.
- Debouncing achieved by short delay per keypress.
- FSM ensures non-blocking transitions between states.

---

Author:
Raymundo
Robotics and Digital Systems Engineer – Tecnológico de Monterrey

License:
Distributed under the MIT License.
