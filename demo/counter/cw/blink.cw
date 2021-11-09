# This file shows a couple of ways to implement a blinker
# the first technique does not use any automatic state 
# transitions and needs to be kick-started since the 
# machine powers-up in the (hidden) INIT state.

Blinker MACHINE {
  EXPORT STATES off, on;
  EXPORT READONLY FLOAT32 counter; 
  OPTION delay 10;
  OPTION counter 0.0;
  on STATE;
  off STATE;

  ENTER on { counter := counter + 0.01;  WAIT delay; SEND turnOff TO SELF; }
  ENTER off { WAIT delay; SEND turnOn TO SELF; }
  off DURING turnOff{}
  on DURING turnOn{}

}
blinker Blinker;

