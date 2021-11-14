# Onscreen keyboard

# The keyboard accepts a key press and appends it to the output
# value. Backspace removes the last item in the output.
#
# The keyboard is designed to return the output value to the
# linked form field when OK is pressed and to clear the output
# when cancel is pressed.

# Humid will need to have a screen with the same name
# as this message box.
keyboard_dialog MessageBox(message: "Enter description");

/*

keyboard_key TEMPLATE
  keyboard_$letter KeyButton(value: $letter) kbd_input;

GENERATE keyboard_key USING { letter RANGE "A" .. "Z" }
GENERATE keyboard_key USING { letter RANGE "0" .. "9" }
GENERATE keyboard_key USING { letter LIST "-","_"," " }
*/

kbd_backspace BackspaceButton kbd_input;
kbd_ok OkButton kbd_input, kbd_output, keyboard_dialog;
kbd_cancel CancelButton keyboard_dialog;
kbd_reset ResetButton kbd_output, kbd_input;

kbd_input TextField; # A copy of the form value as input to the keyboard
kbd_output VARIABLE (export:str, strlen:50) ""; # the result of keyboard entry

kbd_1 KeyButton(value:"1") kbd_input;
kbd_2 KeyButton(value:"2") kbd_input;
kbd_3 KeyButton(value:"3") kbd_input;
kbd_4 KeyButton(value:"4") kbd_input;
kbd_5 KeyButton(value:"5") kbd_input;
kbd_6 KeyButton(value:"6") kbd_input;
kbd_7 KeyButton(value:"7") kbd_input;
kbd_8 KeyButton(value:"8") kbd_input;
kbd_9 KeyButton(value:"9") kbd_input;
kbd_0 KeyButton(value:"0") kbd_input;
kbd_Q KeyButton(value:"Q") kbd_input;
kbd_W KeyButton(value:"W") kbd_input;
kbd_E KeyButton(value:"E") kbd_input;
kbd_R KeyButton(value:"R") kbd_input;
kbd_T KeyButton(value:"T") kbd_input;
kbd_Y KeyButton(value:"Y") kbd_input;
kbd_U KeyButton(value:"U") kbd_input;
kbd_I KeyButton(value:"I") kbd_input;
kbd_O KeyButton(value:"O") kbd_input;
kbd_P KeyButton(value:"P") kbd_input;
kbd_A KeyButton(value:"A") kbd_input;
kbd_S KeyButton(value:"S") kbd_input;
kbd_D KeyButton(value:"D") kbd_input;
kbd_F KeyButton(value:"F") kbd_input;
kbd_G KeyButton(value:"G") kbd_input;
kbd_H KeyButton(value:"H") kbd_input;
kbd_J KeyButton(value:"J") kbd_input;
kbd_K KeyButton(value:"K") kbd_input;
kbd_L KeyButton(value:"L") kbd_input;
kbd_colon KeyButton(value:":") kbd_input;
kbd_Z KeyButton(value:"Z") kbd_input;
kbd_X KeyButton(value:"X") kbd_input;
kbd_C KeyButton(value:"C") kbd_input;
kbd_V KeyButton(value:"V") kbd_input;
kbd_B KeyButton(value:"B") kbd_input;
kbd_N KeyButton(value:"N") kbd_input;
kbd_M KeyButton(value:"M") kbd_input;
kbd_comma KeyButton(value:",") kbd_input;
kbd_period KeyButton(value:".") kbd_input;
kbd_slash KeyButton(value:"/") kbd_input;

