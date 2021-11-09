P_Screen VARIABLE (export:str, strlen:50) "Main";

Counter MACHINE blinker {
  OPTION count 0;
  OPTION step 1;
  OPTION P_Screen "Main";
  OPTION message "";
  EXPORT READONLY 32BIT count;
  EXPORT READONLY STRING 40 message;

  dialog MessageBox(message: "Finished");

  top WHEN count >= 20 && step > 0;
  bottom WHEN count <= 0 && step < 0;
  rising WHEN step > 0;
  falling WHEN step < 0;
  counting DEFAULT;

  ENTER rising { message := "rising"; }
  ENTER falling { message:= "falling"; }

  ENTER top { step := -1 * step; }
  ENTER bottom { step := -1 * step; }

  RECEIVE on_enter FROM blinker { count := count + step; }
  ENTER INIT {
    blinker.delay := 1000;
    SEND turnOn TO blinker;
  }
}
blink Blinker;
counter Counter blink;
