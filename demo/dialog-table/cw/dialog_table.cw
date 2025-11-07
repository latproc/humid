# Dialog table demo combines dialog control with a scrollable table.

P_Dialog VARIABLE (export:str, strlen:50) "dialog_table"; # Instantiated TableDialog
P_DialogVisible VARIABLE(export: rw) 0;
P_Screen VARIABLE (export:str, strlen:50) "MainScreenView";

MessageBox MACHINE {
  OPTION message "Hello World";
  EXPORT STATES invisible, visible;
  EXPORT READONLY STRING 120 message;

  visible STATE;
  invisible INITIAL;

  ENTER visible { P_DialogVisible.VALUE := 1; }
  ENTER invisible { P_DialogVisible.VALUE := 0; }
}

dialog_table MessageBox(message: "Select a person");

DialogController MACHINE dialog {

  COMMAND show {
    SET dialog TO visible;
  }

  COMMAND hide {
    SET dialog TO invisible;
  }
}

dialog_control DialogController dialog_table;

PeopleTableExport MACHINE {
  OPTION value "";
  OPTION selected_row -1;
  EXPORT READWRITE STRING 1000 value;
  EXPORT READWRITE 32BIT selected_row;

  OPTION data JSON_VALUE {
    "header": [],
    "rows": []
  };

  COMMAND sync {
    value := data AS STRING;
  }
}

PeopleTable MACHINE exporter{
  OPTION header_json JSON_VALUE [
      {"label": "Name", "width": 30},
      {"label": "Age", "width": 10},
      {"label": "City", "width": 20}
    ];

  COMMAND sync {
    ITEM ${header} OF exporter.data := header_json;
  }
}

PeopleData MACHINE exporter {
  OPTION people_json JSON_VALUE [
    {"Name": "Alice", "Age": 30, "City": "New York"},
    {"Name": "Bob", "Age": 25, "City": "Los Angeles"},
    {"Name": "Charlie", "Age": 28, "City": "Chicago"},
    {"Name": "Dana", "Age": 32, "City": "Seattle"}
  ];

  COMMAND sync {
    ITEM ${rows} OF exporter.data := people_json;
  }
}

people_table_exporter PeopleTableExport;
people_table PeopleTable people_table_exporter;
people_data PeopleData people_table_exporter;

Sync MACHINE target {
  ENTER INIT { SEND sync TO target; }
}

table_header_sync Sync people_table;
table_data_sync Sync people_data;
