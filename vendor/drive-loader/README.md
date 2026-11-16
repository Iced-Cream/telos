# The Drive Loader's boot linker (vendored)

`dl.h`, `dlink.c`, `json.c` and `json.h` are elekloader's `mods/drive-loader-ar2/` files of the same
names, unchanged, from [elekloader/elekloader](https://github.com/elekloader/elekloader)
(GPL-2.0-or-later; the Analog Rytm MKII's +Drive mods loader, by Iced-Cream). On main, from commit
`50e7e02`, built as they are. On the `poc` branch, from elekloader branch `telos-poc` (188f5ba, not on
main yet), which adds mods that write sites (`DL_SITES`, from the Octatrack's TELOS) and mods that claim
bulk regions (`DL_REGIONS`), each behind its flag; TELOS builds with both.

TELOS links the same `.elemod` files against the build descriptor `core_desc`, and supplies what the
linker asks of its host (`telos.c`: memory, `dl_read_file`, and for sites the stock bytes and the commit).

To update them, copy the four files from a newer elekloader and note its commit here. A change TELOS
needs in them belongs in elekloader first.
