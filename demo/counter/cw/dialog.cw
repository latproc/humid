P_Dialog VARIABLE (export:str, strlen:50) "dialog";
P_DialogVisible VARIABLE(export: ro) 0;

MessageBox MACHINE {
  OPTION message "Hello World";
  EXPORT STATES invisible, visible;
  EXPORT READONLY STRING 120 message;

  visible STATE;
  invisible INITIAL;

  ENTER visible { P_DialogVisible.VALUE := 1; }
  ENTER invisible { P_DialogVisible.VALUE := 0; }
}

