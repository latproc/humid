# Clockwork client licensing — discussion for Martin

**Purpose:** Work through whether / how Clockwork (especially `libcw_client`) can move off a pure GPL footing now that plant EtherCAT access goes through ELC, and what that means for Humid.  
**Audience:** Martin Leadbeater (copyright holder / co-author), Mike.  
**Date:** 2026-08-08  
**Status:** Discussion draft — not a legal opinion or a decided relicense.

Related Humid notes: [THIRD_PARTY.md](../THIRD_PARTY.md).

---

## 1. Why this came up

Humid’s own sources are BSD-style (`LICENSE.txt`). Humid **statically links** Clockwork’s `libcw_client`.

Clockwork / Latproc is labeled **GPL-2 or later** (`clockwork/COPYING`, headers, `LICENCES`).

Under common GPL practice, shipping a **combined humid binary** that includes GPL object code implies **GPL obligations for that combination** (source availability for the client and typically the app that links it). That is awkward next to Humid’s BSD label and is the main “red item” in the Humid third-party audit.

The historical reason for Clockwork being GPL was the **IgH EtherCAT Master** (GPL) in the userspace plant path—not because the HMI messaging client is EtherCAT code.

---

## 2. What we believe today (engineering facts)

### 2.1 `libcw_client` does not contain EtherCAT

In `clockwork/iod/CMakeLists.txt`, `cw_client` is a dedicated library of messaging / value / JSON / logging sources, for example:

- `MessagingInterface`, `ConnectionManager`, `SocketMonitor`, `CommandManager`
- `value`, `symboltable`, `json_expr_*`, `cJSON`
- `Logger`, `Message*`, `watchdog`, `anet`, etc.

It does **not** list:

- `ecat_thread`, `EtherCATSetup`, EtherCAT XML parser  
- SOEM  
- IgH `libethercat` / master userspace  

EtherCAT appears on **other targets** (`iod`, `iod_sdo`, `cw_ecat`, optional SOEM).

Humid only needs **`client-install` / `libcw_client.a`**, not full `iod`.

### 2.2 GPL was driven by the plant EtherCAT stack

Rough historical picture:

```
  [iod userspace] ----linked----> [IgH EtherCAT Master userspace]  (GPL)
         |
         +---- also uses shared Latproc code labeled GPL project-wide
```

So the **project** was GPL, and that label covered everything—including the client humid uses.

### 2.3 ELC changes the architecture

With **ELC** (kernel module) and Etherlab master **in the kernel**, plant access is via a **block device / module boundary**, not by linking IgH master into Clockwork userspace:

```
  [Etherlab master + ELC in kernel]     (kernel / Etherlab rules as applicable)
              ^
              |  block device / ioctl (your interface)
  [iod userspace — ELC backend]         (can be free of libethercat link)
              ^
              |  ZMQ / channels
  [libcw_client]  <---- humid links this
  [humid]
```

That is the right technical boundary if the goal is a **different license for userspace Clockwork** (or at least for the client).

ELC alone does **not** relicense anything; it removes the main **technical** reason userspace had to be GPL.

---

## 3. Issues to work through together

### A. Copyright ownership

Who must agree to any relicense or dual-license?

- Martin Leadbeater (Latproc / COPYING, humid LICENSE)  
- Other named co-authors on Clockwork files (e.g. headers listing multiple authors)  
- Any external contributions merged under GPL assumptions  

**Decision needed:** list of rights-holders for (1) client-only sources, (2) full iod.

### B. Scope of relicense

Options differ a lot by scope:

| Scope | What changes | Effort | Humid impact |
|-------|----------------|--------|--------------|
| **Client only** (`cw_client` + public client headers) | Smallest, clearest for humid | Lower | High (solves panel binary story) |
| **All non-EtherCAT userspace** (cw, tools, ELC iod) | Medium | Medium | High |
| **Whole Clockwork repo** | Includes legacy IgH/SOEM paths | High / may stay impossible for some files | Cleanest branding |

**Recommendation to discuss:** start with **client only** (or dual-license client), keep legacy EtherCAT builds explicitly GPL.

### C. Dual-license vs replace GPL

| Approach | Pros | Cons |
|----------|------|------|
| **Dual: GPL-2+ OR BSD-3** (recipient’s choice) | Old world stays valid; humid can take BSD; less “orphan” risk | Two licenses to maintain on notices |
| **BSD-3 only for client** | Simplest for embeds | Need confidence all client files are free of GPL-only third-party code and all authors agree |
| **Leave GPL, document “internal only”** | No relicense work | External / partner binary distribution remains awkward |

### D. Cleanliness of the ELC plant line

Before calling plant userspace “non-GPL-forced”:

- Confirm default plant `iod` **does not link** `libethercat` / IgH userspace.  
- Confirm no GPL-only sources are compiled into that binary.  
- Keep SOEM / `cw_ecat` / old IgH as **optional legacy targets** with clear GPL labeling.

### E. Third-party code already inside `cw_client`

These are generally fine under BSD/MIT dual (already permissive):

- **cJSON** — MIT  
- **anet** — BSD-style (Redis-era)  
- Boost / ZeroMQ — system libraries (BSL / MPL-2.0)

No IgH in that list.

### F. Humid messaging

Until client license changes:

- Humid `LICENSE.txt` stays BSD for **Humid sources**.  
- `THIRD_PARTY.md` correctly flags **combined binary** obligations from static GPL client.  
- After a client dual/BSD decision, update Humid `LICENSES/Clockwork-*.txt` and THIRD_PARTY.

### G. What we are *not* claiming

- Not claiming kernel Etherlab/ELC can be relicense arbitrarily.  
- Not claiming dual-license is required by law—only that it is a common, practical tool.  
- Not a substitute for lawyer review if code goes to third parties under commercial terms.

---

## 4. Proposed direction (for discussion)

**Phase 1 — Align on intent**

1. Agree that **HMI/panel client** should not be forced GPL solely because of historical IgH userspace.  
2. Agree ELC is the plant EtherCAT path going forward.  
3. Agree **legacy IgH/SOEM** remains a separate, explicitly GPL product line if still needed.

**Phase 2 — Client package definition**

1. Freeze the file list of `cw_client` + installed public headers (`clientlib` install component already exists).  
2. One-page inventory: file → origin → current notice.  
3. Confirm `nm libcw_client.a` has no EtherCAT/IgH symbols (expected already).

**Phase 3 — License decision**

Preferred sketch to debate:

- **`libcw_client` + client headers:** dual-licensed **GPL-2+ OR BSD-3-Clause** (or MIT if you prefer symmetry with other embeds).  
- **`iod` ELC line:** relicense later when plant link line is audited clean.  
- **`iod` + IgH / SOEM / `cw_ecat`:** remain GPL-2+.

**Phase 4 — Execution (only after Phase 3)**

1. Add `LICENSE.CLIENT` (or similar) in Clockwork.  
2. Update notices on client sources/headers only.  
3. Document in Clockwork README / CLIENT_LICENSE.md.  
4. Update Humid THIRD_PARTY + `LICENSES/Clockwork-*`.  
5. Optional: CMake install message for `clientlib` component license.

---

## 5. Questions for Martin

Please mark or reply on each:

1. **Intent:** Should panel/HMI clients be usable under BSD/MIT without treating the whole of Clockwork as GPL-only?  
   - [ ] Yes - [ ] No - [ ] Dual only  

2. **Scope first step:**  
   - [ ] Client library only  
   - [ ] Client + ELC iod  
   - [ ] Entire repo except EtherCAT plugins  

3. **Form:**  
   - [ ] Dual GPL-2+ OR BSD-3  
   - [ ] BSD-3 only for client  
   - [ ] Keep GPL; improve documentation only  

4. **Co-authors:** Who else must sign off on client files?  
   _______________________________________________

5. **Legacy EtherCAT:** Keep IgH/SOEM builds indefinitely as GPL, or schedule removal?  
   - [ ] Keep - [ ] Deprecate - [ ] Remove after date ______  

6. **External distribution:** Do we ship humid/cw_client binaries to anyone outside the company without full source?  
   - [ ] No (internal only) - [ ] Yes (then relicense or GPL compliance process matters more)  

7. **Preferred SPDX for client if dual/BSD:**  
   - [ ] BSD-3-Clause  
   - [ ] MIT  
   - [ ] Other ______  

---

## 6. One-page picture

```
                    KERNEL
         Etherlab master + ELC module
                    │
                    │  block device (no userspace libethercat)
                    ▼
         iod (plant) — ELC backend
                    │  ZMQ / channels
                    ▼
         libcw_client  ◄── humid (panels)
              │
              │  TODAY: labeled GPL with rest of CW
              │  GOAL?:  dual or BSD so humid story is clean
              │
         Legacy (optional, stay GPL):
         iod+IgH, cw_ecat+SOEM, …
```

---

## 7. Suggested next meeting outcome

Minimum useful outcome:

- [ ] Decision on **client dual vs BSD vs status quo**  
- [ ] Named **owners** for inventory + notice edits  
- [ ] Whether **ELC-only plant iod** is a near-term relicense candidate  
- [ ] Whether any **external binary** distribution is in scope  

Optional follow-up (engineering, after decision):

- Scripted file inventory of `cw_client`  
- Draft `LICENSE.CLIENT` text  
- Humid THIRD_PARTY update PR  

---

## 8. References (in-tree)

| Doc | Role |
|-----|------|
| `clockwork/COPYING` | Current project GPL-2 |
| `clockwork/LICENCES` | Latproc GPL note + third-party bits in CW |
| `clockwork/iod/CMakeLists.txt` | `cw_client` vs `iod` / EtherCAT targets |
| Humid `THIRD_PARTY.md` | Combined binary / notice inventory |
| Humid `LICENSE.txt` | Humid BSD-style project license |

---

*Draft for internal discussion. Adjust facts (especially plant ELC link line) if any detail differs on production branches.*
