Humid is a GUI Editor and GUI generator using nanogui.

Humid is primarily intended for use with Clockwork Language
systems (https://github.com/latproc/clockwork).

This is a *work in progress* and still requires significant work.

The general idea is that humid drives a display that can switch
between various 'screens'. Individual fields and buttons on these
screens can be linked to properties within machines within a
clockwork system so that clockwork can control the GUI.

## Branches (this Humid repo)

Panel fleet (1G2C-122 Core/Grab and similar) runs:

  cw-no-ec-tools-compatiblity

That line pins a clockwork/ submodule and builds against
clockwork/iod/stage/lib/libcw_client.a (not a plant /opt/latproc tree).

Older lines:

  production  — conservative historical runtime
  master      — not the panel fleet branch

## Clockwork client dependency (channel / ZMQ)

CHANNEL reconnect, sticky REQ, and cw_client fixes land on clockwork
branch prod-client-zmq-fix (line C), then get pinned into this Humid
branch's clockwork submodule. See:

  docs/CLOCKWORK_CLIENT_BRANCHES.md
  AGENTS.md

Plant iod bus lines (prod-experimental-mqtt-fix, iod-elc) are separate.

## Third-party licenses

Humid is BSD-licensed (see LICENSE.txt). Copies of third-party licenses
for components shipped or linked with Humid are under LICENSES/:

  NanoGUI-LICENSE.txt     NanoGUI (BSD-3)
  libcurl-LICENSE.txt     libcurl (curl/MIT-style)
  RobotoMono-LICENSE.txt  Roboto Mono font (Apache-2.0)
  litehtml-LICENSE.txt    litehtml HTML engine (BSD-3), used by HTMLVIEW
  gumbo-LICENSE.txt       gumbo-parser (Apache-2.0), vendored with litehtml

System libraries such as Cairo, Pango, and Fontconfig (used when HTMLVIEW
is enabled) keep their own distribution licenses from the OS packages.

Thanks for taking the time to look at this project, I hope
to make it more reliable and accessible in the near future.

Martin Leadbeater

