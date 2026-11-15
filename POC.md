# What TELOS needs to load the shop's other mods: four proofs of concept

Branch `poc` here, with elekloader branch `telos-poc` (local, not pushed). All four run in digiemu's
emulator on OS 1.44; none has been on a unit. A last run with this branch's build linked five mods at
once from the store, one of each kind: DigiTide (a region), digihealth (sites), PC SOUND and CHOKE
GROUPS (parameter slots), and GAIN (on fxkit, built in), beside fxkit and DigiHalo in the flashed build.

| | what was missing | the POC | shown in the emulator | left to make it real |
|---|---|---|---|---|
| 1 | Parameter slots: core-dn1 reads `core_params` once, at boot, before TELOS links, so PC SOUND's PRG, CHOKE's CHK and LOOPBACK's LOOP showed `ERR` | core-dn1 5.4 `core_param_install()` (`mods/core-dn1/params.s`): what the boot does for each descriptor, at run time; TELOS calls it after its link, imported weakly | PRG, CHK and LOOP show their names and values and turn (PRG loads pool sound 004) | a test in elekloader, core-dn1 5.4's docs and changelog, then an elekloader PR (core first) |
| 2 | Companions (fxkit, machines-dn1, seq-events): they declare tables, which a +Drive mod cannot | none in code: build them into the firmware with TELOS | GAIN, which needs fxkit, loads from the store with fxkit flashed; its tile and page work | say so in the README and the shop's listing; machines-dn1 and seq-events not tried |
| 3 | Sites: a +Drive mod may not write the OS's code | the Octatrack TELOS's linker (`vendor/drive-loader`, `DL_SITES`, from elekloader's unpushed `telos-ot` branch) with the Digitone's host in `telos.c`: sha256 of each site's bytes against stock, the OS image only (`IMAGE_END` per OS), overlapping sites refused, written with interrupts masked, then the cache flush | digihealth 1.1 from the store: both sites written as `jsr` (0x40000438, 0x400eaaf4), the context-switch hook runs (1,193 calls in about 1.3 emulated seconds), its SYSTEM INFO row and overlay as when flashed (its `CPU --` readout is the emulator's: the flashed build shows the same) | the stack scan before the commit (the Octatrack's `dl_commit_begin`); stock bytes from before the OS ran, for sites in data (the Octatrack copies the data region at `ev_boot_early`); the host twin (`elekloader/drive.py`) and js/ taking sites; `telos-ot`'s linker into elekloader first |
| 4 | Regions: a +Drive mod may not claim bulk memory | `DL_REGIONS` in the linker: a mod's `resources.regions` taken, refused (new code 27) when one overlaps a flashed mod's or an earlier mod's; elekloader's descriptor names flashed mods' regions as claims of kind 2, `<lo> <hi> <name>` | DigiTide from the store beside a flashed DigiHalo: linked byte-identical to elekloader's twin (area and all 11 tables), its tile and page work; a mod overlapping it and one overlapping DigiHalo refused, 27 naming each | the descriptor's kind 2 in docs/DRIVE.md and js/; the host twin taking regions; a check that a region lies in the profile's bulk area (the descriptor could carry the area) |

## Found on the way

- **digiemu's cold boot parks during a long link.** Its first stage stops at the first 10M-instruction
  chunk that is mostly idle and snapshots there. TELOS's link waits on card reads, so with a large file
  (DigiTide's 131 KB) it stopped mid-link, and the next stage never brought the UI up. With 200M chunks
  (`bootstrap.COLD['chunk']`, set in the test harness) the boot finishes. An emulator fix, for digiemu.
- **The crash guard did its job.** Each hung boot left the guard set, and the next boot linked nothing
  (skip 2), until the guard was cleared.
- TELOS now reads a file 16 KB at a time.

## Not tried

- Anything on a unit. Sites and regions both change what the flashed OS runs, so they need the
  hardware test before any release.
- machines-dn1 and seq-events built in; Tone+FX, Mutiny and ESOTERIKA through `DL_SITES` (they may also
  hit sites core-dn1 already owns: refused 24, stock).
