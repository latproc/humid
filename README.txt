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

Humid is BSD-licensed at the project level (see LICENSE.txt). Copies of
third-party licenses for components shipped or linked with Humid are under
LICENSES/. A full inventory and risk notes are in THIRD_PARTY.md.

  NanoGUI-LICENSE.txt       NanoGUI (BSD-3)
  GLFW-LICENSE.txt          GLFW (zlib), via NanoGUI
  NanoVG-LICENSE.txt        NanoVG (zlib), via NanoGUI
  libcoro-LICENSE.txt       libcoro (BSD-2), via NanoGUI
  glad-LICENSE.txt          glad OpenGL loader (MIT), via NanoGUI
  Eigen-MPL2-LICENSE.txt    Eigen headers (MPL-2.0), via NanoGUI
  litehtml-LICENSE.txt      litehtml (BSD-3), HTMLVIEW
  gumbo-LICENSE.txt         gumbo-parser (Apache-2.0), with litehtml
  RobotoMono-LICENSE.txt    Roboto Mono font (Apache-2.0)
  libcurl-LICENSE.txt       libcurl (curl/MIT-style)
  Boost-LICENSE.txt         Boost (BSL-1.0)
  ZeroMQ-MPL2-LICENSE.txt   libzmq (MPL-2.0)
  Clockwork-GPL2-LICENSE.txt  Clockwork libcw_client (GPL-2+)

System libraries such as Cairo, Pango, and Fontconfig (HTMLVIEW) keep
their own licenses from the OS packages (typically LGPL/MIT; dynamic link).

Note: humid statically links Clockwork's client library (GPL-2+). See
THIRD_PARTY.md for distribution implications.

Thanks for taking the time to look at this project, I hope
to make it more reliable and accessible in the near future.

Martin Leadbeater

