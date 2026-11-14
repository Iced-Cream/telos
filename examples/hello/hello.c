/* SPDX-License-Identifier: GPL-2.0-or-later
 * HELLO: a Mod Menu tile from a mod TELOS linked at boot (examples/telos-hello-dn1/mod.json). */
#include "digitone-mk1/core3.h"

static void hello_open(void *brain, void *event, int32_t track);
static void hello_pick(void *brain, void *event, int32_t track, int32_t index);

const struct core_menu_item hello_menu = {"HELLO", hello_open, 0, 0};

static char text[16];
static struct core_menu_item tile = {text, 0, 0, 0};
static const void *list[2] = {&tile, 0};
static int32_t picks;

static void show(void)
{
    int32_t n = picks, k = 0, i;
    char d[8];
    const char *a = "TELOS HI #";
    while (*a)
        text[k++] = *a++;
    i = 0;
    do
        d[i++] = (char)('0' + n % 10);
    while ((n /= 10) && i < 7);
    while (i && k < 14)
        text[k++] = d[--i];
    text[k] = 0;
    core_menu_open(list, 0, hello_pick);
}

static void hello_pick(void *brain, void *event, int32_t track, int32_t index)
{
    (void)brain; (void)event; (void)track; (void)index;
    picks++;
    show();
}

static void hello_open(void *brain, void *event, int32_t track)
{
    (void)brain; (void)event; (void)track;
    show();
}
