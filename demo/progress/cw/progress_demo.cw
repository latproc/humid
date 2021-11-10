# Styles for the progress bar
Pretty MACHINE {
  OPTION progress_fg "#26a";
  OPTION progress_bg "#666";
  EXPORT READONLY STRING 20 progress_fg;
  EXPORT READONLY STRING 20 progress_bg;
}
display Pretty;

# value of the progress bar and its maximum value
pos VARIABLE(export:rw_reg, max_pos: 1000) 0;
max_pos VARIABLE(export:rw_reg) 1000;

# a driver to simulate progress
Ramp MACHINE {
  OPTION label "Testing";
  OPTION progress 0;
  EXPORT RO 16BIT progress;
  EXPORT RW STRING 40 label;

  COMMAND inc { progress := progress + 1; pos := progress }
  COMMAND reset { progress := 0; }
}

Resetter MACHINE ramp {
  reset WHEN ramp.progress > 999;
  ENTER reset { SEND reset TO ramp; }
  idle DEFAULT;
}

Update MACHINE clock, targets {
  RECEIVE clock.on_enter { SEND inc TO targets; }
}

ramp1 Ramp;
ramps LIST ramp1;
update Update clock, ramps;
resetter Resetter ramp1;

Clock MACHINE {
  EXPORT STATES off, on;
  EXPORT READONLY FLOAT32 counter;
  OPTION delay 10;
  OPTION counter 0.0;
  on INITIAL;
  off STATE;

  ENTER on { counter := counter + 0.01;  WAIT delay; SEND turnOff TO SELF; }
  ENTER off { WAIT delay; SEND turnOn TO SELF; }
  off DURING turnOff{}
  on DURING turnOn{}

}
clock Clock;

