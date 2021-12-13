
IncDecButton MACHINE value {
    OPTION step 1;
    OPTION export rw;
    off INITIAL;
    on STATE;

    ENTER on { value := value + step; }
}

# Monitor the given value and prevent out of range values
IncDecButtonMonitor MACHINE value, dec_btn, inc_btn {
    OPTION min 0;
    OPTION max 100;
    fix_min WHEN value < min;
    fix_max WHEN value > max;
    ok WHEN value > min && value < max;
    up_only WHEN value <= min;
    down_only WHEN value >= max;
    
    ENTER ok { ENABLE dec_btn; ENABLE inc_btn; }
    ENTER up_only { DISABLE dec_btn; }
    ENTER down_only { DISABLE inc_btn; }
    ENTER fix_min { value := min; }
    ENTER fix_max { value := max; }
}

# Monitor the selected month and make sure that the days value remains valid
MonthDaysMonitor MACHINE year, month, day, day_monitor {
    leap_feb WHEN month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    feb WHEN month == 2;
    short_month WHEN month == 4 || month == 6 || month == 9 || month == 11;
    long_month DEFAULT;

    ENTER leap_feb { day_monitor.max := 29; }
    ENTER feb { day_monitor.max := 28; }
    ENTER short_month { day_monitor.max := 30; }
    ENTER long_month { day_monitor.max := 31; }
}
month_days_monitor MonthDaysMonitor date_time_year, date_time_month, date_time_day, monitor_day;

date_time_year VARIABLE (export: reg) 2021;
btn_dec_year IncDecButton(export: rw, step:-1) date_time_year;
btn_inc_year IncDecButton (export: rw)date_time_year;
monitor_year IncDecButtonMonitor (min: 1900, max: 2100) date_time_year, btn_dec_year, btn_inc_year;

date_time_month VARIABLE (export: reg) 12;
btn_dec_month IncDecButton(export: rw, step:-1) date_time_month;
btn_inc_month IncDecButton (export: rw)date_time_month;
monitor_month IncDecButtonMonitor (min: 1, max: 12) date_time_month, btn_dec_month, btn_inc_month;

date_time_day VARIABLE (export: reg) 10;
btn_dec_day IncDecButton(export: rw, step:-1) date_time_day;
btn_inc_day IncDecButton (export: rw)date_time_day;
monitor_day IncDecButtonMonitor (min: 1, max: 31) date_time_day, btn_dec_day, btn_inc_day;

date_time_hour VARIABLE (export: reg) 9;
btn_dec_hour IncDecButton(export: rw, step:-1) date_time_hour;
btn_inc_hour IncDecButton (export: rw)date_time_hour;
monitor_hour IncDecButtonMonitor (min: 0, max: 23) date_time_hour, btn_dec_hour, btn_inc_hour;

date_time_min VARIABLE (export: reg) 0;
btn_dec_min IncDecButton(export: rw, step:-1) date_time_min;
btn_inc_min IncDecButton (export: rw)date_time_min;
monitor_min IncDecButtonMonitor (min: 0, max: 59) date_time_min, btn_dec_min, btn_inc_min;

date_time_sec VARIABLE (export: reg) 0;
btn_dec_sec IncDecButton(export: rw, step:-1) date_time_sec;
btn_inc_sec IncDecButton (export: rw)date_time_sec;
monitor_sec IncDecButtonMonitor (min: 0, max: 59) date_time_sec, btn_dec_sec, btn_inc_sec;
