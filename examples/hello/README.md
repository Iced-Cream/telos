# HELLO (TELOS)

The smallest mod for TELOS on the Digitone mk1: kept in the unit's TELOS store
and linked at boot, never flashed. It adds HELLO to the Mod Menu; HELLO opens one tile that counts
its picks.

```bash
python -m elekloader.sdk.build examples/hello --stock <Digitone 1.43 .syx> --out out
python tools/telos.py unit add out/telhello-0.1.elemod
```

A TELOS mod has only `.run` and `.bss`, no sites, and names no firmware address, so its OS 1.44
port is empty (elekloader's docs/FORMAT.md, "+Drive mods").
