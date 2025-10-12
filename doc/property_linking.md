# Property Linking in Humid

Humid allows GUI widget properties to be **linked** to Clockwork machine properties (`OPTION`s).  
This document shows both the **structure** (which components connect) and the **runtime behavior** (how updates flow).

## Defining a Connection

To link Humid to a Clockwork machine, you must define a `CW CONNECTION` in your project settings file (`PROJECTSETTINGS.humid`):

```humid
PROJECTSETTINGS STRUCTURE(
    asset_path: "."
  ) {
  CW CONNECTION(
    channel: PANEL_CHANNEL,
    host: localhost,
    port: 5555
  );
}
ProjectSettings PROJECTSETTINGS(
    asset_path: "."
  );  
```

## Static Structure

The component diagram below shows how widgets in Humid are linked to Clockwork properties via a `CW CONNECTION`.

```plantuml
@startuml
title Humid Property Linking (Static Structure)

left to right direction

package "Humid UI" #D1C4E9 {
  [Progress Bar\n(Remote Object=progress)] as H1
  [Label\n(Remote Object=status_text)] as H2
  [Widget w/Visibility\n(Remote Object=is_visible)] as H3
}

package "CW CONNECTION" #FFF9C4 {
  [CW\nchannel=PANEL_CHANNEL\nhost=localhost\nport=5555] as Conn
}

package "Clockwork Machine" #C8E6C9 {
  [OPTION progress:int] as C1
  [OPTION status_text:string] as C2
  [OPTION is_visible:bool] as C3
}

H1 --> Conn
H2 --> Conn
H3 --> Conn

Conn --> C1
Conn --> C2
Conn --> C3
@enduml
```

# Property Linking in Humid

Humid allows GUI widget properties to be **linked** to Clockwork machine properties (`OPTION`s).  
This document shows both the **structure** (which components connect) and the **runtime behavior** (how updates flow).

---

# Dynamic Behavior

The sequence diagram below shows the flow of a property
update at runtime, from Clockwork through Humid’s linking
mechanism to the widgets.

```plantuml
@startuml
title Flow of a Property Update (Dynamic Behavior)

actor Clockwork
participant "CW CONNECTION" as Conn
participant "LinkManager" as LM
participant "LinkableProperty" as LP
participant "LinkableObject" as LO
participant "PropertyLinkTarget" as PLT
participant "EditorWidget" as EW

Clockwork -> Conn : send updated OPTION value
Conn -> LM : deliver (name, Value)

LM -> LP : lookup property by remote name
LP -> LO : call update(Value) on each linked object

alt direct widget linkage
    LO -> EW : apply value to widget\n(e.g. setProgress, setCaption)
else via PropertyLinkTarget
    LO -> PLT : forward Value
    PLT -> EW : setPropertyValue(name, Value)
end

EW -> EW : redraw / refresh
@enduml
```

- Clockwork sends an updated property value.
- The connection passes it to the LinkManager.
- LinkManager delivers it to the correct LinkableProperty.
- Each linked LinkableObject updates either:
  - the widget directly, or
  - the widget property via a PropertyLinkTarget.
- Finally, the widget redraws to reflect the new value.

