# Control connection overlay

Humid can block runtime interaction while its configured Clockwork connections
are unavailable or are still restoring their initial data snapshot. The feature
is disabled by default and never applies in editing mode.

Enable it on the `ProjectSettings PROJECTSETTINGS(...)` instance:

```text
ProjectSettings PROJECTSETTINGS(
  show_control_disconnected_overlay: true,
  control_disconnected_delay_seconds: 3
);
```

`show_control_disconnected_overlay` enables the overlay only when Humid is run
with `--run_only=1`. `control_disconnected_delay_seconds` defaults to 3 seconds
and may be set to zero for immediate display.

The overlay consumes operator keyboard and mouse input and displays:

```text
Control is not connected
Please wait...
```

It is removed automatically after every configured connection is ready and its
startup/data-refresh state reaches `sDONE`. The current HMI screen is preserved.
