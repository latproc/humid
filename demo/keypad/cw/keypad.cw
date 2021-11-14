# Keypad support

# The keypad accepts a key press and appends it to the output
# value. Backspace removes the last item in the output.
# A period is only valid once within the output so extra
# presses of the period button do nothing.
# A minus is only valid as the first character of the output.
#
# The keypad is designed to return the output value to the
# linked form field when OK is pressed and to clear the output
# when cancel is pressed.

# Humid will need to have a screen with the same name
# as this message box.
keypad MessageBox(message: "Enter value");

keypad_0 KeyButton(value:"0") input;
keypad_1 KeyButton(value:"1") input;
keypad_2 KeyButton(value:"2") input;
keypad_3 KeyButton(value:"3") input;
keypad_4 KeyButton(value:"4") input;
keypad_5 KeyButton(value:"5") input;
keypad_6 KeyButton(value:"6") input;
keypad_7 KeyButton(value:"7") input;
keypad_8 KeyButton(value:"8") input;
keypad_9 KeyButton(value:"9") input;
keypad_period KeyButton(value:".") input;
keypad_minus KeyButton(value: "-" ) input;
keypad_backspace BackspaceButton input;
keypad_ok OkButton input, output, keypad;
keypad_cancel CancelButton keypad;
keypad_reset ResetButton output, input;

input TextField; # A copy of the form value as input to the keypad
output VARIABLE (export:str, strlen:50) ""; # the result of keypad entry
period_button_monitor PeriodMonitor input, keypad_period; # validating period
minus_button_monitor MinusMonitor input, keypad_minus; # validating minus

