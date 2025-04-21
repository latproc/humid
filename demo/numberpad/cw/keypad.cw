# Keypad support

P_Dialog VARIABLE (export:str, strlen:50) "dialog";
P_DialogVisible VARIABLE(export: ro) 0;

dialog MessageBox(message: "Enter value");

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
keypad_ok OkButton input, output;
keypad_cancel CancelButton dialog;
keypad_reset ResetButton output, input;

input TextField;
output VARIABLE (export:str, strlen:50) "";
period_button_monitor PeriodMonitor input, keypad_period;
minus_button_monitor MinusMonitor input, keypad_minus;

KeyButton MACHINE input {
    OPTION value "";
    OPTION export rw;
    off INITIAL;
    on STATE;

    ENTER on { input.value := input.value + value; }
}

BackspaceButton MACHINE input {
    OPTION export rw;
    off INITIAL;
    on STATE;
    ENTER on { input.value := COPY `(.*).` FROM input.value }
}

TextField MACHINE {
    OPTION value "";
    EXPORT READWRITE STRING 50 value;
}

OkButton MACHINE input, output {
    OPTION export rw;
    off INITIAL;
    on STATE;
    ENTER on {
        output.VALUE := input.value;
        input.value := ""
    }
}

ResetButton MACHINE source, dest {
    OPTION export rw;
    off INITIAL;
    on STATE;
    ENTER on {
        dest.value := source.VALUE;
    }
}

CancelButton MACHINE keypad {
    COMMAND cancel { SET keypad TO invisible; }
}

PeriodMonitor MACHINE input, button {
    # There can only be one period (full-stop) in a value
    OPTION last "";
    idle WHEN last == input.value;
    disabling WHEN input.value MATCHES `.*\..*` AND button ENABLED;
    enabling WHEN input.value MATCHES `^[^.]*$` AND button DISABLED;
    updating DEFAULT;
    ENTER disabling { DISABLE button; }
    ENTER enabling { ENABLE button; }
    ENTER updating { last := input.value; }
}

MinusMonitor MACHINE input, button {
    # A minus character can only be entered as the first character in the field
    OPTION last "";
    idle WHEN last == input.value;
    disabling WHEN input.value MATCHES `.` AND button ENABLED;
    enabling WHEN input.value MATCHES `^$` AND button DISABLED;
    updating DEFAULT;
    ENTER disabling { DISABLE button; }
    ENTER enabling { ENABLE button; }
    ENTER updating { last := input.value; }
}

MessageBox MACHINE {
  OPTION message "Hello World";
  EXPORT STATES invisible, visible;
  EXPORT READONLY STRING 120 message;

  visible STATE;
  invisible INITIAL;

  ENTER visible { P_DialogVisible.VALUE := 1; }
  ENTER invisible { P_DialogVisible.VALUE := 0; }
}

