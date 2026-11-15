# The Drive Loader's boot linker (vendored)

`dl.h`, `dlink.c`, `json.c` and `json.h` are elekloader's `mods/drive-loader-ar2/` files of the same
names, unchanged, from [elekloader/elekloader](https://github.com/elekloader/elekloader) at commit
`50e7e02` (GPL-2.0-or-later; the Analog Rytm MKII's +Drive mods loader, by Iced-Cream). On the `poc` branch, `dl.h`
and `dlink.c` are the Octatrack TELOS's (elekloader branch `telos-ot`, 874e7c5, not on main), built
with `DL_SITES` (mods that write sites). On main TELOS builds them
as they are, without `DL_SITES`: it links the same `.elemod` files against the build descriptor
`core_desc`, and supplies what the linker asks of its host (`telos.c`: memory, `dl_read_file`).

To update them, copy the four files from a newer elekloader and note its commit here. A change TELOS
needs in them belongs in elekloader first.
