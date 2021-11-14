# Keypad buttons and support machines

# Humid requires the name of the dialog to display and a flag
# to indicate when it should be visible.
hmi_dialog VARIABLE (export:str, strlen:50) "dialog";
hmi_dialog_visible VARIABLE(export: ro) 0;

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

OkButton MACHINE input, output, dialog {
    OPTION export rw;
    off INITIAL;
    on STATE;
    ENTER on {
        output.VALUE := input.value;
        input.value := "";
        SET dialog TO invisible;
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

CancelButton MACHINE dialog {
    COMMAND cancel { SET dialog TO invisible; }
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

  ENTER visible { P_Dialog := SELF.NAME; P_DialogVisible.VALUE := 1; }
  ENTER invisible { P_DialogVisible.VALUE := 0; }
}

