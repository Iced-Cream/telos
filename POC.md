# What TELOS needs to load the shop's other mods: four proofs of concept

Branch `poc` here, with elekloader branch `telos-poc` (local, not pushed). All run in digiemu's emulator
on OS 1.44; a hardware pack is in `digitaktresearch/telos-poc-hw` (not yet on a unit).

| | what was missing | the POC | shown in the emulator | left to make it real |
|---|---|---|---|---|
| 1 | Parameter slots: core-dn1 reads `core_params` once, at boot, before TELOS links, so PC SOUND's PRG, CHOKE's CHK and LOOPBACK's LOOP showed `ERR` | core-dn1 5.4 `core_param_install()` (`mods/core-dn1/params.s`): what the boot does for each descriptor, at run time; TELOS calls it after its link, imported weakly | PRG, CHK and LOOP show their names and values and turn (PRG loads pool sound 004) | a test in elekloader, core-dn1 5.4's docs and changelog, then an elekloader PR (core first) |
| 2 | Companions: they declare tables, which a +Drive mod cannot | none in code: build them into the firmware with TELOS | each of the three with a mod that needs it, from the store: fxkit and GAIN (tile, page, active effect), machines-dn1 and SINE, seq-events and EUCLID; also Mutiny (seq-events, with the shop's copy too) and ESOTERIKA's Xtal (needs ESOTERIKA, itself from the store) | say so in the README and the shop's listing |
| 3 | Sites: a +Drive mod may not write the OS's code | the Octatrack TELOS's linker (`vendor/drive-loader`, `DL_SITES`, from elekloader's unpushed `telos-ot` branch) with the Digitone's host in `telos.c`: each site's bytes checked against stock (sha256), the OS image only (`IMAGE_END` per OS), written with interrupts masked, then the cache flush; and elekloader's descriptor carries the stock shadow (`drive_shadow`, ABI 1.2), the stock bytes of every range the flashed build writes | loaded and booted from the store: digihealth (sites written as `jsr`, its hook runs, SYSTEM INFO as when flashed), ESOTERIKA (+ Xtal), Mutiny (its tile), Tone+FX (loads and boots; its UI not checked). Refused as they should be: a site over another stored mod's (21), Tone+FX beside ESOTERIKA (21: they patch the same places), a site over a core-dn1 site (22, flashed; 24, stock, without the shadow) | the stack scan before the commit (the Octatrack's `dl_commit_begin`); stock bytes from before the OS ran, for sites in data; the shadow and sites in the host twin (`elekloader/drive.py`) and js/; `telos-ot`'s linker into elekloader first |
| 4 | Regions: a +Drive mod may not claim bulk memory | `DL_REGIONS` in the linker: a mod's `resources.regions` taken, refused (new code 27) when one overlaps a flashed mod's or an earlier mod's; elekloader's descriptor names flashed mods' regions as claims of kind 2, `<lo> <hi> <name>` | DigiTide from the store beside a flashed DigiHalo: linked byte-identical to elekloader's twin (area and all 11 tables), its tile and page work; a mod overlapping it and one overlapping DigiHalo refused, 27 naming each | the descriptor's kind 2 in docs/DRIVE.md and js/; the host twin taking regions; a check that a region lies in the profile's bulk area |

## Found on the way

- **Stock parts need the shadow.** TELOS copies a mod's stock parts (the stock instructions a site
  displaces, carried in its `.run`) from RAM, where the flashed build's own sites are. Tone+FX (1 part)
  and Mutiny (2 parts) overlap core-dn1 sites: linked without the shadow they crashed at the project
  load (a jump to an odd address in the flashed build); flashed, the PC linker copies from the stock
  file and they run. The shop's other TELOS-loadable mods have no such part.
- **digiemu's cold boot parks during a long link.** Its first stage stops at the first 10M-instruction
  chunk that is mostly idle and snapshots there; TELOS's link waits on card reads, so with large files it
  stopped mid-link and the next stage never brought the UI up. With 200M chunks
  (`bootstrap.COLD['chunk']`, set in the test harness) the boot finishes. An emulator fix, for digiemu.
- **The crash guard did its job.** Each crashed or hung boot left the guard set, and the next boot
  linked nothing (skip 2) until it was cleared.
- TELOS now reads a file 16 KB at a time.

## Not tried

- Anything on a unit (the pack in `telos-poc-hw` covers 1-4 with mods that need no shadow).
- Tone+FX's own screens.
