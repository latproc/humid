# Humid File Rules

These notes describe the `.humid` files observed in `config/` and `Screens/`.
They are based on the current files and on validation with `/opt/humid/build/hmifile_check`.

This workspace is SVN-only. Do not assume git metadata or git workflow is available here; use the working files and SVN context instead.

## Project Scope

This project is much simpler than the source examples these notes were derived from.

- Target screen size is `1280x800`.
- New screen declarations should use `screen_width: 1280` and `screen_height: 800`.
- System display size should use `w:1280` and `h:800` when creating or updating project config.
- Do not add image or camera UI for this project.
- Avoid `IMAGE` widgets, `.imageURL` remotes, and camera navigation buttons unless the project requirements explicitly change.

## Current Project Learnings

These points come from reworking `Screens/INITIAL.humid` and
`Screens/IOCORE.humid` against the live `humid` display and checking the
result with screenshots.

- Prefer screenshot-based verification, not just `hmifile_check`. A file can
  validate and still render badly.
- The preferred screenshot path is now native capture from `humid` itself.
  Use that method on the laptop build instead of relying on VNC or
  Wayland-specific tooling.
- The basic capture command is:

```sh
~/src/github/humid/build/humid --capture /tmp/out.png --screen Initial config Screens
```

- Run that command from the project directory that contains the local
  `config/` and `Screens/` directories.
- Change `Initial` to the required screen name when capturing another page.
- The earlier working screenshot capture path in this environment was
  Wayland-native and should now be treated as fallback only:

```sh
runuser -u hmi -- env XDG_RUNTIME_DIR=/run/user/1001 WAYLAND_DISPLAY=wayland-0 grim /tmp/screen.png
```

- The native capture path renders directly from `humid`, so prefer it over
  restart-and-wait screen grabs when available.

- The current `humid` process is restarted externally. When a screen edit needs
  a fresh render, killing `humid` is enough and the supervisor restarts it.
- After restart, do not assume a valid screenshot is immediately available.
  Wait for `humid` to be fully running before capture.
- `V_MACHINESHORTDESC` is the correct title binding for this project, even if it
  temporarily shows `0` before the export is wired up.
- For this project, replace donor `Grab` bindings with `Core` bindings where
  the underlying machine is now core-only. `INITIAL.humid` was updated from old
  `Grab` remotes to `Core` remotes taken from `referece_CW_code/io.lpc` and
  `referece_CW_code/globals.lpc`.
- Prefer derived/aggregate status names from `globals.lpc` when they exist.
  Example: use `F_CoreAirPressureOK` for the air-pressure status on the screen,
  not the raw analog input.
- For fault banners, gate each row from its own fault signal rather than trying
  to hide a group of rows with a broader power-status signal like `I_CoreE24V`.
  On `INITIAL.humid`, the reliable pattern was:
  - `I_CoreMotorBreaker` is an OK signal, so the alarm banner must invert it
    and only show when the point is off/bad.
  - `F_CoreAirPressureOK` is also an OK signal, so the air-pressure alarm must
    invert it and only show when the point is off/bad.
- The bale counters for this project are `V_CoreTotalNoBales` and
  `V_CoreTotalBalesToday`.
- A good `1280x800` layout for this project uses fewer panels, wider status
  rows, and removes donor navigation such as camera/service buttons.
- On `INITIAL.humid`, the bale counters read better when split into the two top
  corners instead of stacking both counters on the left.
- For the same screen, the remaining non-navigation content works better as one
  centered block:
  - safety gate column at `x=180`, width `430`
  - EStop column at `x=670`, width `430`
  - shared instruction/alarm rows at `x=180`, width `920`
- Leave the `Core IO` navigation button anchored at the bottom-left. That
  button is functional navigation, not part of the centered status layout.

## Panel Screen Contract

The current panel-facing screen routing comes from
`referece_CW_code/panel.lpc`. The panel `ScreenNo` names are the routing
contract; the `.humid` files need to match those screen purposes even if the
internal humid structure names differ.

Current normal screen flow:

1. `Initial`
2. `MotorStart`
3. mode select through `P_CoreMode`
4. `CoreChamberResume`
5. `CoreSetNewBaleDetailsAtLoader`
6. `ManualControls`

Current jump screens:

- `P_CoreJumpAutoSettings` -> `CoreAutoSettings`
- `P_CoreJumpManual` -> `ManualControls`
- `P_CoreJumpAuto` -> `CoreSetNewBaleDetailsAtLoader`
- `P_CoreJumpIO` -> `CoreIO`

## Screen Purposes

- `Initial`
  - Startup/status landing screen while E24 is not yet in a ready/running
    state.
  - Show machine identity, bale counters, gate/EStop diagnostic state, and the
    current fault/warning banners that matter before startup.
  - Keep the core I/O jump available at bottom-left.

- `MotorStart`
  - Second startup screen after `Initial`, entered before the auto/manual fork.
  - This screen exists to request the main core motor start through the panel
    flag `P_CoreMotorStart`, not by sending a direct machine command.
  - Keep it minimal. A title, the start button, and the core I/O jump are
    enough if the upstream guard/screen flow is correct.
  - Keep the core I/O jump available at bottom-left.

- `CoreIO`
  - Service/diagnostic page for current core-side I/O state.
  - Prefer real physical I/O from `referece_CW_code/io.lpc` plus a small number
    of useful aggregate statuses such as `I_Core24V` and `I_CoreE24V`.
  - Keep the return button available at bottom-right through
    `M_CoreControl2Panel.Reset`.
  - This page is now generated for `1280x800` from:
    - source list: `Buildtools/CoreIO.txt`
    - generator: `Buildtools/generate_core_io.py`
  - Maintenance flow for this screen is:
    1. edit `Buildtools/CoreIO.txt`
    2. regenerate `Screens/IOCORE.humid`
    3. run `/opt/humid/build/hmifile_check Screens/IOCORE.humid`
    4. verify with a screenshot from live `humid`
  - The input list should be ordered by functional machine groups, not just raw
    PLC/module order.
  - Analog inputs belong at the end of the input list.
  - The older `Buildtools/IOLayout.php` remains a donor/reference script for
  the old large-screen layout. Do not extend it for this project.

## Border And Valign Notes

Live rendering found some important behavior that is not obvious from the
checker alone.

- Static descriptive text should usually use `border: 0`.
  Example: headings like `Safety Gate Status` and `EStop Status`.
- `valign` behavior is widget-dependent in this renderer. Do not assume the
  same numeric value means the same visual alignment for `LABEL`, `BUTTON`, and
  `INDICATOR`.
- For this project, many `LABEL` widgets render too low with `valign: 2`.
  `IOCORE.humid` rendered better with `valign: 1` for title labels, section
  labels, and row text labels.
- `INDICATOR` widgets in `IOCORE.humid` kept `valign: 2`.
- Buttons should use `border: 2`.
- Wrapped paragraph-style text may need `valign: 1` instead of `valign: 2`.
  The `GateEStop_Directions` label rendered better that way.
- `border: 2` is valid and renders correctly on the gate and EStop status
  indicators in `INITIAL.humid`.
- Do not assume `border: 2` is safe everywhere. A previous broader change
  produced a blank render even though `hmifile_check` still passed. If using
  `border: 2`, apply it incrementally and verify with a screenshot after each
  group of changes.

## File Types

There are two broad file types:

- `config/*.humid` files define project/system configuration objects.
- `Screens/*.humid` files define HMI screens and the widgets inside them.

All current files validate with:

```sh
/opt/humid/build/hmifile_check config/*.humid Screens/*.humid
```

## Top-Level Shape

A typical file has one object declaration and one matching `STRUCTURE` block.

```humid
CoreIO IOCORE(file_name: "IOCORE.humid",screen_height: 800,screen_width: 1280);
IOCORE STRUCTURE EXTENDS SCREEN {
  Title LABEL(caption: "Example",font_size: 60,height: 54,pos_x: 0,pos_y: 0,width: 1280);
}
```

The object declaration uses:

```humid
InstanceName TYPE(property: value,property: value);
```

The structure block uses:

```humid
TYPE STRUCTURE {
}
```

or, for screens:

```humid
TYPE STRUCTURE EXTENDS SCREEN {
}
```

The declaration and structure do not have to appear in that order. Several valid screen files place the `STRUCTURE` first and the object declaration at the end, for example:

```humid
COREBLOCKEDTUBE STRUCTURE EXTENDS SCREEN {
  ...
}
CoreBlockTube COREBLOCKEDTUBE(file_name: "COREBLOCKEDTUBE.humid",screen_height: 800,screen_width: 1280);
```

The important relationship is that the declaration type and the structure name match.

## Statements

Most statements end with semicolons.

- Top-level object declarations end with `;`.
- Widget declarations end with `;`.
- `OPTION` declarations end with `;`.
- `STRUCTURE` blocks end with `}` and no semicolon.

Blank lines are accepted. Indentation is flexible; files use spaces and some tabs.

Comments are accepted with `#`. One observed example comments out a property at the end of a widget line:

```humid
... wrap: 1); #visibility: "M_CoreScalesCapture.stable"
```

## Properties

Properties use `name: value` syntax inside parentheses. Properties are comma-separated.

Observed value types:

- Strings in double quotes: `caption: "Return"`.
- Integers: `width: 1280`.
- Floats: `value_scale: 1.000000`.
- Booleans: `inverted_visibility: false`.
- Color strings as quoted RGBA floats: `bg_color: "0.0996,0.9431,0.2180,1.0000"`.

Property ordering is not fixed. For example, `bg_color` and `bg_on_color` appear in different orders in different widgets.

Whitespace around parentheses and commas is flexible. `DIALOG.humid` uses:

```humid
Dialog DIALOGSCREEN( file_name: "DIALOG.humid", screen_height: 800, screen_width: 1280);
```

Long widget declarations are usually written on one line, but multi-line property lists are valid. `DIALOG.humid` uses a multi-line `FRAME`.

## Screen Declarations

Screen declarations include:

- `file_name`: usually the display file name.
- `screen_height`: `800` for this project.
- `screen_width`: `1280` for this project.

Example:

```humid
GrabInitial INITIALGRABSCREEN(file_name: "INITIALGRAB.humid",screen_height: 800,screen_width: 1280);
```

## 1280x800 Layout Notes

- This project must be laid out explicitly for `1280x800`; donor `1920x1080`
  screens do not scale down cleanly.
- Reduce density before trying to preserve donor spacing.
- Avoid decorative or unused controls. This machine has no image/camera
  workflow.
- On IO screens, tighter row heights are acceptable when needed to keep all
  required points on one screen.
- Grouping by machine function reads better than grouping strictly by terminal
  module order.

One observed quirk: `Screens/INITIAL.humid` declares `file_name: "INITIALGRAB.humid"`, so the `file_name` value is not guaranteed to equal the actual disk filename.

## Config Structures

`config/System.humid` defines a `SYSTEM` object and an empty `SYSTEM STRUCTURE`.

```humid
System SYSTEM(active_screen: "",remote_screen: "V_GrabControl2HMI",full_screen:1, w:1280, h:800);
SYSTEM STRUCTURE {
}
```

`config/ProjectSettings.humid` defines project settings and a connection inside its structure.

```humid
ProjectSettings PROJECTSETTINGS(project_history: 500000);
PROJECTSETTINGS STRUCTURE {
  Core CONNECTION(channel: "PANEL_CORE_CHANNEL",host: "172.29.53.1",port: 5555);
}
```

## Screen Contents

Screen structures contain child declarations in this form:

```humid
WidgetName WIDGETTYPE(property: value,property: value);
```

Observed widget types:

- `LABEL`
- `BUTTON`
- `INDICATOR`
- `TEXT`
- `FRAME`

`IMAGE` exists in the larger source examples but should not be used in this project.

`CONNECTION` appears inside project settings, not screen structures.

## Common Widget Properties

Layout and sizing:

- `pos_x`
- `pos_y`
- `width`
- `height`

Text and display:

- `caption`
- `font_size`
- `alignment`
- `valign`
- `wrap`
- `border`
- `text`
- `format`

Live data and control binding:

- `connection`
- `remote`
- `command`
- `visibility`
- `inverted_visibility`
- `value_type`
- `value_scale`

Colors:

- `bg_color`
- `bg_on_color`
- `text_colour`
- `on_text_colour`

State text:

- `on_caption`

Images:

- `image_file`
- `scale`

These image properties are documented only because they exist in the source examples. They are out of scope for this project.

## Connections And Remotes

Widgets bind to PLC or remote values with `connection` and `remote`.

Examples:

```humid
Title_Label LABEL(connection: "Grab",remote: "V_MACHINESHORTDESC",...);
CoreScalesWeight LABEL(connection: "Core",remote: "P_CoreScalesWeight.Weight",...);
```

Observed connection names include `Core`, `Grab`, and uppercase `CORE`. The checker accepts the case used in the files, but new files should follow the case already used near the copied pattern unless there is a known reason to change it.

`remote` values are dot-path-like strings. Common suffixes include:

- `.Jump`
- `.Reset`
- `.Yes`
- `.No`
- `.Weight`
- `.used`
- `.off`
- `.blocked`

`.imageURL` is present in the source examples but is out of scope here because this project has no images or cameras.

## Commands

Buttons can use either `remote` or `command`.

Observed command forms:

```humid
command: "SET P_CoreMode TO on"
command: "SET P_CoreMode TO off"
command: "SEND Start TO M_CoreMotor"
command: "SEND toggle TO F_Location6Tube01"
command: "SEND Copy TO P_CoreCopyGrabExit2Loader"
```

The command language appears string-based and is passed through as the property value.

## Options And Function Keys

Some screens declare function-key mappings at the top of the structure:

```humid
OPTION KEY_F1 "AutoModeSwitch";
OPTION KEY_F2 "ManualJump";
OPTION KEY_F5 "WeightOKScales";
OPTION KEY_F6 "WeightOKScales";
```

The quoted value names a widget in the same structure.

## Visibility Pattern

Visibility is controlled by a remote-like string:

```humid
visibility: "F_CoreLoaderUp"
```

`inverted_visibility` flips the visibility logic when set to `true`.

Many screens layer widgets at the same coordinates and use visibility to choose which one is visible. For example, a background indicator, an "Interlocked" label, and an active button can share the same area.

## Button And Indicator State

`BUTTON` and `INDICATOR` commonly use:

- `behaviour`
- `caption`
- `on_caption`
- `bg_color`
- `bg_on_color`
- `text_colour`
- `on_text_colour`

The numeric `behaviour` values are accepted as-is. Observed values include `0`, `1`, `4`, and `64`.

## Layout Conventions

Screens are designed for `1280x800`.

Observed conventions:

- Main title labels usually sit at the top, around `pos_y: 0` to `pos_y: 70`.
- Navigation/action buttons often sit near the bottom edge of the `800` pixel display.
- Manual control screens use repeated blocks made from a background `INDICATOR`, an interlock `LABEL`, a `BUTTON`, and a label.
- IO screens use repeated `INDICATOR` plus `LABEL` pairs in columns.
- Do not use image-heavy screen patterns in this project.

Coordinates are absolute pixels. Widgets can overlap intentionally.

## Naming Conventions

Names are identifier-like and use letters, numbers, and underscores.

Common patterns:

- Screen types are uppercase: `IOCORE`, `WEIGHTCONFIRM`.
- Top-level instances are PascalCase or descriptive: `CoreIO`, `WeightConfirm`.
- Widgets use descriptive names: `Title_Label`, `JumpCoreIO`, `P_CoreLoaderUp`.
- Related widget groups share prefixes, for example `P_CoreLoaderUp_BG`, `P_CoreLoaderUp_Interlocked`, `P_CoreLoaderUp`, `P_CoreLoaderUp_Label`.

## Practical Validation Rule

After editing or creating a file, run:

```sh
/opt/humid/build/hmifile_check path/to/file.humid
```

For a full project check, run:

```sh
/opt/humid/build/hmifile_check config/*.humid Screens/*.humid
```

The checker reports loaded files and structures. A clean run for the current project reports all 17 files and 17 structures.

## Project Backlight Control

Humid supports an optional project-level backlight integration. Configure it
on the `ProjectSettings PROJECTSETTINGS(...)` declaration, not in an operator
screen or a machine-specific Humid widget:

```humid
ProjectSettings PROJECTSETTINGS(
  backlight_interface: "sysfs",
  backlight_path: "/sys/class/backlight/10-0027",
  backlight_on_brightness: 255,
  backlight_control_point: "P_CoreHmiBacklightEnabled",
  backlight_off_delay_seconds: 300
);
```

- `backlight_control_point` is an exported local Clockwork point. Humid reads
  it during the initial Clockwork refresh and responds to later changes.
  `on` restores the backlight immediately; `off` starts the delay.
- `backlight_off_delay_seconds` is measured in seconds. Use a short value
  such as `30` while commissioning, then set the operational value.
- `backlight_interface` may be `sysfs`, `edatec-ddc`, `ddcutil`, `x11-dpms`,
  `wayland-dpms`, `wlr-randr`, or `none`.
  - `sysfs` writes brightness under `backlight_path`; use it for this EDATEC
    integrated panel. It writes `0` when off and restores
    `backlight_on_brightness` when on. Do not toggle `bl_power` on this raw
    DSI driver: its LEDs resume but its video image may not.
  - `edatec-ddc` runs the installed `ed-ddc-server` brightness command; use
    it only where that tool and a DDC-capable display are available.
  - `ddcutil` talks to the monitor over DDC/CI. The original D6 power
    codes remain: `backlight_ddc_on_value` (default `01` On) and
    `backlight_ddc_off_value` (default `03` Suspend). Brands that implement
    02/03 still get that. `04`/`05` (hard off) are refused because they drop
    HDMI on Valleyview i915. In addition, Humid blanks with **brightness
    VCP `10` = 0** and restores the saved value on wake. Disable the extra
    with `backlight_ddc_brightness_feature: "none"`. Set
    `backlight_ddc_bus` when more than one `/dev/i2c-N` may be present.
    A key or mouse button wakes a blanked display before the disconnected
    overlay consumes input. Operator activity restarts `backlight_off_delay_seconds`.
  - `x11-dpms` uses `xset` to force the X11 display on, or into DPMS
    **standby** when blanking. Never `force off`: DPMS Off / VCP D6=`04`
    drops HDMI on Dell P-series / Valleyview i915. Blanking also sets
    `xset dpms 0 0 0` so the server cannot escalate standby to off after
    10 minutes. While the control point is on, automatic DPMS stays
    disabled (`xset -dpms`). Humid must inherit `DISPLAY` and X authority.
  - `wayland-dpms` uses `wlopm` to power the configured Wayland output on or
    off. Set `backlight_output` to the compositor output name, for example
    `HDMI-A-1`; Humid must inherit `XDG_RUNTIME_DIR`. Install `wlopm` on the
    target system before selecting this interface.
    If the launcher removes `WAYLAND_DISPLAY` to force Humid through Xwayland,
    set `backlight_wayland_display` if the compositor socket is not the default
    `wayland-0`. Humid supplies that value only to the `wlopm` child process.
  - `wlr-randr` enables or disables the configured wlroots output. Use it on a
    wlroots compositor such as labwc when `wlopm` is unavailable. It uses the
    same `backlight_output` and optional `backlight_wayland_display` settings as
    `wayland-dpms`.
  - `none` disables physical backlight writes while retaining the configured
    point.

For an X11 HDMI HMI (non-Dell, or until DDC is available):

```humid
ProjectSettings PROJECTSETTINGS(
  backlight_interface: "x11-dpms",
  backlight_control_point: "P_CoreHmiBacklightEnabled",
  backlight_off_delay_seconds: 300
);
```

For Dell P-series on Intel Valleyview, D6 `03` is not in the EDID
capability list (`01`/`04`/`05` only). Keep D6 on=`01` off=`03` (no-op
on these Dells) and rely on the brightness blank. Requires `ddcutil` 2.x
(Bookworm: `2.2.0-1+bookworm1`), `i2c-dev`, and `hmi` access to `/dev/i2c-*`.

```humid
ProjectSettings PROJECTSETTINGS(
  backlight_interface: "ddcutil",
  backlight_ddc_bus: 5,
  backlight_ddc_on_value: "01",
  backlight_ddc_off_value: "03",
  backlight_control_point: "P_CoreHmiBacklightEnabled",
  backlight_off_delay_seconds: 300
);
```

For a labwc/Wayland HDMI HMI:

```humid
ProjectSettings PROJECTSETTINGS(
  backlight_interface: "wayland-dpms",
  backlight_output: "HDMI-A-1",
  backlight_control_point: "P_CoreHmiBacklightEnabled",
  backlight_off_delay_seconds: 300
);
```

The configured point must be exported by Clockwork, for example:

```lpc
P_CoreHmiBacklightEnabled PFLAG (tab:Panel, export:rw);
```

### Linux Permission Rule

Humid normally runs as the unprivileged `hmi` user. On this panel the sysfs
backlight attributes are created root-owned, so the following root-owned
tmpfiles rule is required for the `sysfs` interface:

`/etc/tmpfiles.d/humid-backlight.conf`

```text
z /sys/class/backlight/10-0027/bl_power 0666 root root -
z /sys/class/backlight/10-0027/brightness 0666 root root -
```

Apply it immediately and on subsequent boots with:

```sh
systemd-tmpfiles --create /etc/tmpfiles.d/humid-backlight.conf
```

Only `brightness` is required by the current sysfs implementation; retaining
`bl_power` access is harmless but optional. Do not rely on a udev `GROUP`/`MODE` rule for this case: it does not change
the permissions of these individual sysfs attributes. Limit the tmpfiles rule
to the exact backlight device used by the project.
