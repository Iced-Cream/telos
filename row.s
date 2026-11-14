| SPDX-License-Identifier: GPL-2.0-or-later
| TELOS (Digitone mk1): the SETTINGS row MODS, which opens TELOS's page (page.c).
| ColdFire V4; -mcpu=54455.
|
|   STR_CTOR    std::string(dest, const char *, alloc&): the firmware's (mod.json's defsym,
|               1.44's in its port; core-dn1 uses the same one in params.s)
|
| A row is four callbacks in the firmware's MenuItem conventions (core's settings.s,
| core_additem): label() -> std::string through a0, select(payload, item), draw(payload, item,
| bmp, x, y), change(payload, item, delta). As digihealth's SYSTEM INFO row, written again.

        .section .run, "ax"
        .balign 2

| ev_settings handler: add the row.
        .globl  telos_settings
telos_settings:
        pea     telos_row
        move.l  8(%sp), -(%sp)          | the menu
        jsr     core_additem
        addq.l  #8, %sp
        rts

| label() -> std::string, built in place at a0 (the caller's)
row_label:
        link    %a6, #-4
        move.l  %d2, -(%sp)
        pea     -1(%a6)                 | the allocator
        pea     str_label
        move.l  %a0, %d2
        move.l  %a0, -(%sp)
        jsr     STR_CTOR
        lea     12(%sp), %sp
        move.l  %d2, %d0
        move.l  -8(%a6), %d2
        unlk    %a6
        rts

| select(payload, item): open the page
row_select:
        jmp     telos_page_open

| draw(): the gear icon (page.c, telos_row_draw); change(): LEFT and RIGHT do nothing
row_change:
        rts

        .balign 4
telos_row:  .long   row_label, row_select, telos_row_draw, row_change
str_label:  .asciz  "MODS (TELOS)"
        .balign 4
