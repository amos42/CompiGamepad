"""
" Edit below this line to fit your needs
"""
# Path to pngview (raspidmx) and icons
PNGVIEWPATH = "/opt/retropie/configs/all/CompiGamepad"
ICONPATH = "/opt/retropie/configs/all/CompiGamepad"

# Fully charged voltage, voltage at the percentage steps and shutdown voltage. This is where you edit when finetuning the batterymonitor
# by using the monitor.py script.
VOLT100 = 4.1
VOLT75 = 3.76
VOLT50 = 3.63
VOLT25 = 3.5
VOLT0 = 3.2

# Value (in ohms) of the lower resistor from the voltage divider, connected to the ground line (1 if no voltage divider). 
# Default value (2000) is for a lipo battery, stepped down to about 3.2V max.
LOWRESVAL = 2000

# Value (in ohms) of the higher resistor from the voltage divider, connected to the positive line (0 if no voltage divider).
# Default value (5600) is for a lipo battery, stepped down to about 3.2V max.
HIGHRESVAL = 5600

# ADC voltage reference (3.3V for Raspberry Pi)
ADCVREF = 5

# MCP3008 channel to use (from 0 to 7)
ADCCHANNEL = 0

# Refresh rate (s)
REFRESH_RATE = 2

# Display some debug values when set to 1, and nothing when set to 0
DEBUGMSG = 0
