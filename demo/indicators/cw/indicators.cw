# toggle some indicator lights

Clock MACHINE {
  OPTION delay 600;

  idle DEFAULT;
  tick WHEN TIMER >= delay;
}


ToggleOnOff MACHINE clock {
  on STATE;
  off INITIAL;

  RECEIVE clock.tick_enter { SEND tick TO SELF; }

  TRANSITION on TO off ON tick;
  TRANSITION off TO on ON tick;
}

ToggleTrueFalse MACHINE clock {
  true STATE;
  false INITIAL;

  RECEIVE clock.tick_enter { SEND tick TO SELF; }

  TRANSITION true TO false ON tick;
  TRANSITION false TO true ON tick;
}

clock1 Clock;

i_1 ToggleOnOff(export:ro) clock1;
i_2 ToggleOnOff(export:ro) clock1;
i_3 ToggleOnOff(export:ro) clock1;
i_4 ToggleTrueFalse(export:ro) clock1;
i_5 ToggleTrueFalse(export:ro) clock1;
i_6 ToggleTrueFalse(export:ro) clock1;

