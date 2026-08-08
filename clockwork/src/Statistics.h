/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __STATISTICS_H
#define __STATISTICS_H 1

#include "Statistic.h"

struct Statistics {
    Statistic io_scan_time;
    Statistic points_processing;
    Statistic machine_processing;
    Statistic dispatch_processing;
    Statistic auto_states;
    Statistic web_processing;

    Statistics()
        : io_scan_time("I/O Scan            "), points_processing("POINTS sync         "),
          machine_processing("Machine Processing  "), dispatch_processing("Dispatch Processing "),
          auto_states("Stable States       "), web_processing("Web Processing      ") {}
};

#endif
