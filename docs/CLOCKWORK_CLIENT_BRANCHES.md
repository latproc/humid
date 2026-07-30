# Clockwork client lines (for Humid developers)

**Updated:** 2026-07-31  
**Humid branch this applies to:** `cw-no-ec-tools-compatiblity`  
  (1G2C-122 Core/Grab panels: `172.29.58.10` / `.11` port **2222**)  
**Audience:** CHANNEL / ZMQ / sticky REQ / `cw_client` work for Humid

Humid does **not** own plant EtherCAT backends. It links Clockwork **client**
code (`cw_client`, `ConnectionManager`, channel setup REQ/REP) via the
**`clockwork/` git submodule** pinned by this Humid commit.

On a **docs-only** host (e.g. `/opt/humid` without a full panel toolchain), do
not expect a green build. Build on a panel or a matching release host, then
deploy the panel binary.

## Humid branch vs Clockwork lines

| Repo | Branch | Role |
|------|--------|------|
| **humid** | `cw-no-ec-tools-compatiblity` | Panel fleet Humid (this line) |
| humid | `production` / `master` | Older / other lines — not what 1G2C-122 panels run |
| **clockwork** **A** | `prod-experimental-mqtt-fix` | Plant `iod_sdo` (legacy ecrt) |
| **clockwork** **B** | `feature/iod-elc-kernel-transport` | Plant `iod-elc` (kernel) |
| **clockwork** **C** | `prod-client-zmq-fix` | **Canonical trunk for client/ZMQ** (`scope: client-zmq`) |

Clockwork port matrix: `iod/docs/BRANCHES.md` and
`iod/docs/LEGACY_ECRT_REMOVAL_PLAN.md` in the clockwork tree.

## Panel build path (required)

Prefer the **submodule** client — never a random plant `/opt/latproc` tree:

```bash
cd /opt/humid
# after submodule points at the intended clockwork SHA:
cd clockwork/iod && make client-install
cd /opt/humid/build && cmake ..   # Found: .../clockwork/iod/stage/lib/libcw_client.a
cmake --build . --target humid -- -j4
```

Or: `./scripts/update-panel.sh` (see `AGENTS.md`).

`FindClockworkClient.cmake` forces the submodule stage path when
`clockwork/iod/stage/lib/libcw_client.a` exists.

## Where to land CHANNEL / sticky REQ fixes

1. Develop on clockwork **C** (`prod-client-zmq-fix`) with `scope: client-zmq`.
2. Commit and push **clockwork** (operator approval).
3. On Humid: check out that commit (or a merge of it) on a local branch in
   the `clockwork/` submodule, test `make client-install` + humid rebuild,
   then **pin** the submodule SHA in Humid and commit on
   `cw-no-ec-tools-compatiblity`.
4. Port monorepo overlap into clockwork **A** and **B** so plant trees do not
   ship stale shared sources.
5. Deploy **Humid** (and the pinned client) to panels — plant `iod` /
   `iod-elc` restart alone does not fix a sticky client REQ.

Do **not** track new humid-client work only on A (`prod-experimental-mqtt-fix`)
anymore; A remains the plant legacy line. C is the client trunk (cut from A).

## Commit tags (clockwork)

| Tag | Humid relevance |
|-----|-----------------|
| `scope: client-zmq` | Channel setup, ConnectionManager, REQ recreate, `cw_client` |
| `scope: iod-core` | Shared headers/JSON — rebuild client if you include them |
| `scope: bus-elc` / `scope: bus-legacy` | Plant bus only — ignore for Humid |

Port line: `Port of <hash> from <branch>: <one line>`.

## Explicit non-goals

- Confusing humid `cw-no-ec-tools-compatiblity` with clockwork `prod-client-zmq-fix`.
- Building Humid against plant `/opt/latproc` when the submodule client exists.
- Proving deploy from a docs-only WC without a panel rebuild.
