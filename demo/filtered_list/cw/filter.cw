# This program demonstrates filtering a list.
#
datafile CONSTANT (export:str, strlen: 30) "data/endangered.dat";
tempfile CONSTANT "/tmp/filtered.dat";
script CONSTANT "scripts/filter";

filter FilterMonitor;

FilterMonitor MACHINE {
  OPTION filter "";
  OPTION filename "";
  EXPORT READWRITE STRING 50 filter;
  EXPORT READONLY STRING 50 filename;

  LOCAL OPTION last_filter "";
  LOCAL OPTION safe_filter "";

  filter_script SystemExec;

  idle DEFAULT;
  start_search STATE;

  # grep sets an error code if the value can't be found (Errors is blank)
  error WHEN filter_script IS Error && filter_script.Errors != "";
  reload_data WHEN filter_script IS Done || filter_script IS Error;
  start_search WHEN SELF IS update AND filter_script IS Idle AND filter MATCHES `[a-zA-Z0-9_-]{3}`;
  INIT WHEN SELF IS update AND filter_script IS Idle AND NOT filter MATCHES `[A-Za-z0-9_-]`;
  update WHEN filter != last_filter;

  ENTER update {
    last_filter := filter;
  }

  ENTER start_search {
    safe_filter := COPY ALL `[A-Za-z0-9_-]` FROM filter;
    filter_script.Command := script + " " + datafile + " " + safe_filter + " " + tempfile;
    SEND start TO filter_script
  }

  ENTER reload_data {
    filename := "";
    filename := tempfile;
    SET filter_script TO Idle;
  }

  ENTER error {
    LOG "filter script failed with error: " + filter_script.Errors;
    SET filter_script TO Idle;
  }

  ENTER INIT {
    filter_script.Command := "/bin/cp " + datafile + " " + tempfile;
    SEND start TO filter_script;
  }

}

