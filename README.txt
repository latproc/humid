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

  master

That line vendors the trimmed Clockwork client sources under clockwork/src
and builds against clockwork/stage/lib/libcw_client.a (not a plant
/opt/latproc tree).

Older lines:

  production  — conservative historical runtime
  cw-no-ec-tools-compatiblity — legacy pinned-submodule deployment line

## Clockwork client dependency (channel / ZMQ)

CHANNEL reconnect, sticky REQ, and cw_client fixes land on clockwork
branch prod-client-zmq-fix (line C), then are pulled into Humid's vendored
clockwork/src tree with clockwork/compare-clockwork-src.sh. See:

  docs/CLOCKWORK_CLIENT_BRANCHES.md
  AGENTS.md

Plant iod bus lines (prod-experimental-mqtt-fix, iod-elc) are separate.

Thanks for taking the time to look at this project, I hope
to make it more reliable and accessible in the near future.

Martin Leadbeater
