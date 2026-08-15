# Control connection overlay

Humid can block runtime interaction while its configured Clockwork connections
are unavailable or are still restoring their initial data snapshot. The feature
is disabled by default and never applies in editing mode.

Enable it on the `ProjectSettings PROJECTSETTINGS(...)` instance:

```text
ProjectSettings PROJECTSETTINGS(
  show_control_disconnected_overlay: true,
  control_disconnected_delay_seconds: 3,
  control_overlay_settle_frames: 1
);
```

`show_control_disconnected_overlay` enables the overlay only when Humid is run
with `--run_only=1`. `control_disconnected_delay_seconds` defaults to 3 seconds
and applies only after a previously-ready session drops; a cold start (Humid
up while Clockwork is still down, or the first connection of the process)
shows the overlay immediately. The delay may be set to zero for immediate
display on every drop. `control_overlay_settle_frames` defaults to 1 and is
the number of fully painted frames that must complete while the overlay still
covers the rebuilt page before it is removed.

The overlay consumes operator keyboard and mouse input and displays:

```text
Control is not connected
Please wait...
```

It is removed only after every configured connection is ready, its
startup/data-refresh state reaches `sDONE`, the Clockwork-selected HMI screen
is loaded (or the current local screen if `active_screen` is unset), snapshot
values have been applied to that page, and at least one covered frame has been
painted. The current HMI screen is preserved.
