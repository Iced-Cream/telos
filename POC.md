# What TELOS needs to load the shop's other mods: four proofs of concept

Branch `poc` here, with elekloader branch `telos-poc` (local, not pushed; TELOS vendors its boot linker from it). All run in digiemu's emulator
on OS 1.44. The hardware pack `digitaktresearch/telos-poc-hw` (TP2B) has all of it: in the emulator ten
mods, from every addition, load from the store in one boot. It has not been on a unit yet.

| | what was missing | the POC | shown in the emulator | left for a release |
|---|---|---|---|---|
| 1 | Parameter slots: core-dn1 reads `core_params` once, at boot, before TELOS links, so PC SOUND's PRG, CHOKE's CHK and LOOPBACK's LOOP showed `ERR` | core-dn1 5.4 `core_param_install()` (`mods/core-dn1/params.s`): what the boot does for each descriptor, at run time; TELOS calls it after its link (weak import) | PRG, CHK and LOOP show their names and values and turn (PRG loads pool sound 004) | a core test, core-dn1 5.4's docs and changelog, an elekloader PR |
| 2 | Companions: they declare tables, which a +Drive mod cannot | none in code: built into the firmware with TELOS | each with a mod that needs it, from the store: fxkit + GAIN (tile, page, active effect), machines-dn1 + SINE, seq-events + EUCLID and Mutiny (with elekloader's seq-events and the shop's copy); ESOTERIKA + Xtal | the README and the shop's listing say so |
| 3 | Sites: a +Drive mod may not write the OS's code | the Octatrack TELOS's linker (`vendor/drive-loader`, `DL_SITES`) with the Digitone's host in `telos.c`: each site checked against stock (sha256), inside the OS image (`IMAGE_END`), not over the flashed build (the descriptor's **stock shadow**, `drive_shadow`, ABI 1.2), not over another mod's; **written only when no task is stopped inside a site** (the ten static tasks' stacks scanned, from core-dn1 5.4's `core_tasks`, 50 tries, else 26); stock bytes from **core-dn1 5.4's copy of the OS made before it ran** (`core_stock_info`) | from the store: digihealth (sites as `jsr`, hook runs, SYSTEM INFO), ESOTERIKA + Xtal, Mutiny (tile), Tone+FX (its pages: Internal Mixer 1/12, RING, PERFORM, WRMZ DELAYS). Refused as they should be: 21 site over another's (sitedup; Tone+FX beside ESOTERIKA), 22 over a core site, 24 not stock, 26 a site over a waiting task's return address (commit after 51 tries: none; boot goes on). A data site the OS changes before the join: 24 without the pre-OS copy, loads with it | done on the branches: the linker's sites and regions are in elekloader's `mods/drive-loader-ar2` behind `DL_SITES`/`DL_REGIONS` (the Rytm loader builds the same code without them); the task table is core-dn1's `core_tasks`; js/ checks sites. Left: an elekloader PR |
| 4 | Regions: a +Drive mod may not claim bulk memory | `DL_REGIONS`: a mod's `resources.regions`, refused (27) over a flashed mod's (the descriptor's claims of kind 2, `<lo> <hi> <name>`) or an earlier mod's | DigiTide from the store beside a flashed DigiHalo (byte-identical to elekloader's twin, area and all 11 tables; tile and page); overlaps of each kind refused 27 | a region-in-bulk check (docs/DRIVE.md has kind 2; js/ checks regions) |

**Host side.** elekloader's boot-linker twin (`elekloader/drive.py`) takes sites and regions on a build
with the shadow, rule for rule (`tests/test_drive.py`, `test_telos_sites_and_regions`; FORMAT.md lists
codes 21-27); on the nine-mod emulator set it gives the unit's results exactly (6 load; sitedup 21,
corehit 22, Tone+FX 21). The Rytm's twin is unchanged. The TypeScript engine (the web page's checker)
does the same: `js/src/desc.ts` reads and writes the shadow and region claims, `js/src/drive.ts` takes
sites and regions (`js/test/drive.test.ts`, ten cases from the Python), and a TP2B build is byte-identical
to the Python's. From the descriptor alone (`/mods/.build`) both leave the stock hash to the unit; on
TP2B's descriptor both load all ten of the pack's mods.

## Found on the way

- **Stock parts need the shadow.** TELOS copies a mod's stock parts from RAM, where the flashed build's
  own sites are; Tone+FX (1 part) and Mutiny (2) overlap core-dn1 sites and crashed until the shadow.
- **The OS changes 1,238 bytes of its image before the join** (data from 0x4018fefc on, core's moved
  tables included): hence core's copy before the OS runs.
- **digiemu's cold boot parks during a long link** (it snapshots at the first mostly idle 10M-instruction
  chunk; TELOS waits on card reads). With 200M chunks the boot finishes. An emulator fix, for digiemu.
- **The crash guard did its job**: every crashed or hung boot was followed by one that linked nothing.
- TELOS reads a file 16 KB at a time.
