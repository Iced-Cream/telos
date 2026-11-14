| SPDX-License-Identifier: GPL-2.0-or-later
| TELOS (Digitone mk1): the cache flush after the link. ColdFire V4; -mcpu=54455.
|
| dl_flush: push every data-cache line to memory, then invalidate the instruction and branch
| caches, as elekloader's mods/drive-loader-ar2/flush.s does on the Rytm (the same core). CPUSHL takes a set
| and way in An, not an address: 512 sets x 4 ways. Then CACR 0xA40CE100: the Digitone's own
| value (0xA50CE100, set at 0x4000055a; research/telos-dn1, S7) without DCINVA, so no dirty line
| is dropped.
        .section .run, "ax"
        .balign 2
        .globl  dl_flush
dl_flush:
        move.l  #0, %a0
        move.l  #512 * 4, %d1
1:      cpushl  %dc, (%a0)
        addq.l  #1, %a0
        move.l  %a0, %d0
        and.l   #3, %d0
        bne.s   2f
        lea     12(%a0), %a0            | way 3 done: the next set (+16, ways back at 0)
2:      subq.l  #1, %d1
        bne.s   1b
        move.l  #0xA40CE100, %d0
        movec   %d0, %cacr
        nop
        rts
