# Wiring Notes

- Float sensors use `INPUT_PULLUP` mode.  
  Wire one side of each float to **GND** and the other to the pin.
  - Closed (water present) → reads **LOW** if `FLOAT_ACTIVE_LOW = true`.

- Relay module is usually **active LOW** (LOW = ON).  
  Change `RELAY_ACTIVE_LOW` in code if yours differs.

- LEDs connect through resistors:
  - Red → GPIO 14  
  - Blue → GPIO 27  
  - Green → GPIO 12  

- Adjust pins in `main.cpp` if your wiring differs.
