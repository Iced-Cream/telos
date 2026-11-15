/* SPDX-License-Identifier: GPL-2.0-or-later */
/* TELOS: a project's mods.
 *
 * Every project save carries the mods that were running (a core_projdata block, "TLMS": per mod a hash of
 * its id and one of its version). When a project is loaded, TELOS compares that list with what runs now and
 * says, one screen each, in this order:
 *
 *   LOAD FOR THIS PROJECT   mods the project used that are in the store but off: YES turns them on and
 *                           restarts; NO goes on
 *   NOT ON THIS UNIT        mods the project used that neither the store nor the firmware has (or that the
 *                           boot refused: their E code)
 *   CHANGED SINCE SAVE      a mod running at another version than the project's: YES continues anyway, NO
 *                           opens MODS (TELOS)
 *   NOT IN THIS PROJECT     mods running now that the project was not saved with
 *
 * The boot's own load of the last project says nothing, unless TELOS restarted to load a project's mods:
 * then it says only what is still missing. A project saved without the block (before TELOS, or with the
 * project area full) says nothing either.
 *
 * The names come from TELOS's index on the +Drive (sectors +17..+31 of the store): the id, version and
 * title of every mod TELOS has met, in the store or built into the firmware, kept after a mod is removed
 * so a missing mod still has a name. A store file is indexed from its first sector (the elekloader writer
 * puts "id", "version" and "title" first), once.
 *
 * The block goes last in the project area (order 95): when the area is full, the pedals' settings keep
 * their room and the project has no list. */
#include "digitone-mk1/core3.h"
#include "telos.h"

#define KEY_TEMPO   9
#define KEY_PLAY    11
#define KEY_STOP    12
#define KEY_YES     13
#define KEY_NO      14
#define EV_KEY(ev)   (*(uint32_t *)((char *)(ev) + 12))
#define EV_FLAGS(ev) (*(uint32_t *)((char *)(ev) + 16))

typedef int32_t (*text_fn)(void *bmp, const void *font, int32_t x, int32_t y, int32_t maxlen, const char *fmt, ...);
#define TEXTF ((text_fn)fw_textf)

extern const char core_desc[];
void telos_page_open(void);
extern volatile uint8_t telos_dir_busy;

/* ---- the index ------------------------------------------------------------------------------------- */
#define IX_HEAD     17u                  /* the header sector: "TLX2", the count, the pending list */
#define IX_FIRST    18u                  /* the entries: sectors +18..+31 */
#define IX_MAX      112
#define PEND_MAX    48

struct ix_entry {                        /* 64 bytes */
    u32 crc;                             /* the store file's CRC-32; 0: none (built in, or removed) */
    u32 id, ver;                         /* dl_hash of the id and of the version */
    char sid[20];                        /* the id, the version, the title (each NUL-terminated, cut) */
    char sver[8];
    char title[24];
};

static struct {
    char magic[4];                       /* "TLX2" */
    u32 n;
    u32 pending;                         /* pairs in pend: the set a TELOS restart is to load */
    u32 pad;
    u32 pend[PEND_MAX * 2];
    u8 tail[512 - 16 - PEND_MAX * 8];
} ixh __attribute__((aligned(16)));
static struct ix_entry ix[IX_MAX] __attribute__((aligned(16)));
static int ix_ok;
static u8 sec[512] __attribute__((aligned(16)));

static u32 hash(const char *s)
{
    u32 n = 0;
    while (s[n])
        n++;
    return dl_hash(s, n);
}

static void copy(char *d, const char *s, u32 max, int upper)
{
    u32 i;
    for (i = 0; i + 1 < max && s[i]; i++)
        d[i] = (char)(upper && s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i]);
    d[i] = 0;
}

static void ix_load(void)
{
    if (ix_ok)
        return;
    if (fw_card_read(TS_SECTOR + IX_HEAD, 512, &ixh) == 0 && ixh.magic[0] == 'T' && ixh.magic[1] == 'L'
        && ixh.magic[2] == 'X' && ixh.magic[3] == '2' && ixh.n <= IX_MAX && ixh.pending <= PEND_MAX
        && fw_card_read(TS_SECTOR + IX_FIRST, sizeof ix, ix) == 0) {
        ix_ok = 1;
        return;
    }
    dl_memset(&ixh, 0, sizeof ixh);
    dl_memset(ix, 0, sizeof ix);
    ixh.magic[0] = 'T'; ixh.magic[1] = 'L'; ixh.magic[2] = 'X'; ixh.magic[3] = '2';
    ix_ok = 1;
}

static void ix_save(void)
{
    if (fw_card_write(TS_SECTOR + IX_HEAD, 512, &ixh) == 0)
        fw_card_write(TS_SECTOR + IX_FIRST, sizeof ix, ix);
}

static const struct ix_entry *by_crc(u32 crc)
{
    u32 i;
    for (i = 0; i < ixh.n; i++)
        if (ix[i].crc == crc)
            return &ix[i];
    return 0;
}

/* The entry for (id, ver); ver 0: any version, the last met. */
static const struct ix_entry *by_id(u32 id, u32 ver)
{
    int i;
    for (i = (int)ixh.n - 1; i >= 0; i--)
        if (ix[i].id == id && (!ver || ix[i].ver == ver))
            return &ix[i];
    return 0;
}

/* Keep (crc, id, version, title): an entry of its own, or the oldest one no store file holds. */
static int ix_put(u32 crc, const char *id, const char *ver, const char *title)
{
    u32 hi = hash(id), hv = hash(ver), i;
    struct ix_entry *e = 0;
    for (i = 0; i < ixh.n; i++)
        if (ix[i].id == hi && ix[i].ver == hv && (ix[i].crc == crc || !ix[i].crc || !crc)) {
            if (ix[i].crc == crc || !crc)
                return 0;                /* known already */
            e = &ix[i];                  /* a removed file's entry comes back */
            break;
        }
    if (!e && ixh.n < IX_MAX)
        e = &ix[ixh.n++];
    if (!e) {                            /* full: the oldest no store file holds goes */
        for (i = 0; i < IX_MAX && ix[i].crc; i++)
            ;
        if (i == IX_MAX)
            return 0;
        for (; i + 1 < IX_MAX; i++)
            ix[i] = ix[i + 1];
        e = &ix[IX_MAX - 1];
    }
    e->crc = crc;
    e->id = hi;
    e->ver = hv;
    copy(e->sid, id, sizeof e->sid, 0);
    copy(e->sver, ver, sizeof e->sver, 1);
    copy(e->title, title && title[0] ? title : id, sizeof e->title, 1);
    return 1;
}

/* A string value's bytes at p (after its opening quote) to d: -> past its closing quote, or 0. */
static const u8 *str_at(const u8 *p, const u8 *end, char *d, u32 max)
{
    u32 k = 0;
    while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end)
            p++;
        if (k + 1 < max)
            d[k++] = (char)*p;
        p++;
    }
    d[k] = 0;
    return p < end ? p + 1 : 0;
}

/* A store file's id, version and title from its first sector: the top-level string values of those keys
 * before anything else nests (the writer puts them first). -> 1 with an id and a version. */
static int head_of(const struct ts_entry *e, char *id, char *ver, char *title)
{
    const u8 *p = sec, *end = sec + 512;
    char key[12];
    id[0] = ver[0] = title[0] = 0;
    if (fw_card_read(TS_SECTOR + e->sector, 512, sec) != 0)
        return 0;
    while (p < end && *p != '{')
        p++;
    for (p++; p < end;) {
        while (p < end && (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t'))
            p++;
        if (p >= end || *p != '"')
            break;
        p = str_at(p + 1, end, key, sizeof key);
        while (p && p < end && (*p == ' ' || *p == ':'))
            p++;
        if (!p || p >= end)
            break;
        if (*p == '"') {
            char v[28];
            p = str_at(p + 1, end, v, sizeof v);
            if (!p)
                break;
            if (!dl_strcmp(key, "id"))
                copy(id, v, 28, 0);
            else if (!dl_strcmp(key, "version"))
                copy(ver, v, 16, 0);
            else if (!dl_strcmp(key, "title"))
                copy(title, v, 24, 0);
        } else if ((*p >= '0' && *p <= '9') || *p == '-' || *p == 't' || *p == 'f' || *p == 'n') {
            while (p < end && *p != ',' && *p != '}')
                p++;
        } else {
            break;                       /* an object or a list: the head is over */
        }
        if (id[0] && ver[0] && title[0])
            break;
    }
    return id[0] && ver[0];
}

/* Every store file and every module built into the firmware, in the index. */
static void ix_update(void)
{
    u32 i, n, pool, mods;
    int changed = 0;
    char id[28], ver[16], title[24];
    ix_load();
    if (telos_dir_ok)
        for (i = 0; i < telos_dir.n; i++) {
            const struct ts_entry *e = &telos_dir.e[i];
            if (by_crc(e->crc))
                continue;
            if (!head_of(e, id, ver, title)) {   /* not a file the writer made: its name stands in */
                copy(id, e->name, sizeof id, 0);
                copy(ver, "?", sizeof ver, 0);
                title[0] = 0;
            }
            changed |= ix_put(e->crc, id, ver, title[0] ? title : e->title);
        }
    pool = dl_rd32((const u8 *)core_desc + 0x18);
    n = dl_rd16((const u8 *)core_desc + 0x20);
    mods = dl_rd32((const u8 *)core_desc + 0x28);
    for (i = 0; i < n; i++) {
        const u8 *m = (const u8 *)core_desc + mods + 16 * i;
        changed |= ix_put(0, core_desc + pool + dl_rd32(m), core_desc + pool + dl_rd32(m + 4), 0);
    }
    if (changed)
        ix_save();
}

/* ---- the sets ------------------------------------------------------------------------------------- */
#define SET_MAX     32
struct set { u32 n, id[SET_MAX], ver[SET_MAX]; };

static struct {                          /* the project's block: what a save writes */
    u32 used;                            /* CORE_PROJDATA_VARIABLE: bytes held, this word included */
    u32 magic;                           /* "TM01" */
    u32 n;
    u32 pair[SET_MAX * 2];               /* id, version */
} pdata __attribute__((aligned(4)));

static struct set run;                   /* what runs: built-in mods (not core, not TELOS) and the store's */

static int has(const struct set *s, u32 id)
{
    u32 i;
    for (i = 0; i < s->n; i++)
        if (s->id[i] == id)
            return (int)i;
    return -1;
}

static void add(struct set *s, u32 id, u32 ver)
{
    if (s->n < SET_MAX && has(s, id) < 0) {
        s->id[s->n] = id;
        s->ver[s->n++] = ver;
    }
}

static int ran(int i) { return (telos_boot_flags[i >> 5] >> (i & 31)) & 1; }

static void run_build(void)
{
    u32 i, n, pool, mods;
    run.n = 0;
    pool = dl_rd32((const u8 *)core_desc + 0x18);
    n = dl_rd16((const u8 *)core_desc + 0x20);
    mods = dl_rd32((const u8 *)core_desc + 0x28);
    for (i = 0; i < n; i++) {
        const u8 *m = (const u8 *)core_desc + mods + 16 * i;
        if (m[8] != 1)                   /* flashed mods only: core and the loader are the firmware */
            continue;
        add(&run, hash(core_desc + pool + dl_rd32(m)), hash(core_desc + pool + dl_rd32(m + 4)));
    }
    if (telos_dir_ok)
        for (i = 0; i < telos_dir.n; i++) {
            const struct ix_entry *x;
            if (!ran((int)i) || telos_boot_st[i].state != DL_ST_LOADED)
                continue;
            x = by_crc(telos_dir.e[i].crc);
            if (x)
                add(&run, x->id, x->ver);
        }
    pdata.magic = 0x544d3031u;           /* "TM01" */
    pdata.n = run.n;
    for (i = 0; i < run.n; i++) {
        pdata.pair[2 * i] = run.id[i];
        pdata.pair[2 * i + 1] = run.ver[i];
    }
    pdata.used = 12 + 8 * run.n;
}

/* ---- the check ------------------------------------------------------------------------------------ */
enum { P_LOAD, P_MISSING, P_CHANGED, P_NEW, P_N };
#define LIST_MAX    12
static struct {
    u8 n;
    u8 e[LIST_MAX];                      /* P_LOAD: store entries; others: unused */
    char line[LIST_MAX][22];
} pg[P_N];
static int page = -1;                    /* the screen shown, or -1 */
static u8 eat;                           /* a key whose release the dialog still takes */
static u8 dirty;
static u32 boot_ticks;
static int boot_load;                    /* the next load is the boot's own */
volatile u32 telos_pj[4];                /* [0] loads seen, [1] the last check's counts (4 bits each), [2] found */

static void line(int p, const char *a, const char *b)
{
    char *d;
    u32 k = 0;
    if (pg[p].n >= LIST_MAX)
        return;
    d = pg[p].line[pg[p].n++];
    while (*a && k < 21)
        d[k++] = *a++;
    if (b)
        while (*b && k < 21)
            d[k++] = *b++;
    d[k] = 0;
}

/* The store entry that holds the id (the saved version's if there is one), or -1. */
static int store_of(u32 id, u32 ver)
{
    int i, best = -1;
    if (!telos_dir_ok)
        return -1;
    for (i = 0; i < telos_dir.n; i++) {
        const struct ix_entry *x = by_crc(telos_dir.e[i].crc);
        if (x && x->id == id) {
            if (x->ver == ver)
                return i;
            best = i;
        }
    }
    return best;
}

static void next_page(void)
{
    for (page++; page < P_N && !pg[page].n; page++)
        ;
    if (page >= P_N)
        page = -1;
    dirty = 1;
}

static void check(const u32 *pair, u32 n, int quiet_new)
{
    u32 i;
    struct set saved;
    char e2[22];
    for (i = 0; i < P_N; i++)
        pg[i].n = 0;
    saved.n = 0;
    for (i = 0; i < n && i < SET_MAX; i++)
        add(&saved, pair[2 * i], pair[2 * i + 1]);
    for (i = 0; i < saved.n; i++) {
        u32 id = saved.id[i], ver = saved.ver[i];
        int r = has(&run, id), s;
        const struct ix_entry *x = by_id(id, ver), *any = x ? x : by_id(id, 0);
        const char *name = any ? any->title : "UNKNOWN MOD";
        if (r >= 0) {
            const struct ix_entry *now = by_id(id, run.ver[r]);
            if (run.ver[r] != ver) {     /* TONE+FX 4.3B>4.4 */
                char v[22];
                u32 k = 0;
                const char *a = x ? x->sver : "?", *b = now ? now->sver : "?";
                while (*a && k < 9)
                    v[k++] = *a++;
                v[k++] = '>';
                while (*b && k < 19)
                    v[k++] = *b++;
                v[k] = 0;
                line(P_CHANGED, name, 0);
                line(P_CHANGED, "  ", v);
            }
            continue;
        }
        s = store_of(id, ver);
        if (s >= 0 && !(telos_dir.e[s].flags & TS_ON)) {
            if (pg[P_LOAD].n < LIST_MAX)
                pg[P_LOAD].e[pg[P_LOAD].n] = (u8)s;
            line(P_LOAD, name, 0);
        } else if (s >= 0 && ran(s) && telos_boot_st[s].state == DL_ST_REFUSED) {
            u32 c = telos_boot_st[s].code;  /* on, and the boot refused it */
            e2[0] = ' '; e2[1] = 'E';
            e2[2] = (char)(c >= 10 ? '0' + c / 10 : '0' + c);
            e2[3] = (char)(c >= 10 ? '0' + c % 10 : 0);
            e2[4] = 0;
            line(P_MISSING, name, e2);
        } else if (s >= 0) {
            line(P_MISSING, name, " +ON");  /* on from the next boot */
        } else {
            line(P_MISSING, name, 0);
        }
    }
    if (!quiet_new)
        for (i = 0; i < run.n; i++)
            if (has(&saved, run.id[i]) < 0) {
                const struct ix_entry *x = by_id(run.id[i], run.ver[i]);
                line(P_NEW, x ? x->title : "UNKNOWN MOD", 0);
            }
    telos_pj[1] = pg[0].n | pg[1].n << 4 | pg[2].n << 8 | pg[3].n << 12;
    page = -1;
    next_page();
}

static u32 last_pair[SET_MAX * 2], last_n;   /* the loaded project's list, for LOAD's restart */

static void keep(void)
{
    u32 i;
    last_n = pdata.n < SET_MAX ? pdata.n : SET_MAX;
    for (i = 0; i < 2 * last_n; i++)
        last_pair[i] = pdata.pair[i];
}

static void proj_loaded(int32_t found)
{
    int boot = boot_load;
    telos_pj[0]++;
    telos_pj[2] = (u32)found;
    boot_load = 0;
    ix_update();                         /* files sent since the boot */
    if (boot) {
        if (ixh.pending) {               /* TELOS restarted to load a project's mods: what is still missing */
            check(ixh.pend, ixh.pending, 1);
            ixh.pending = 0;
            ix_save();
        }
    } else if (found && pdata.magic == 0x544d3031u) {
        keep();
        check(pdata.pair, pdata.n, 0);
    }
    run_build();                         /* a save writes what runs */
}

const struct core_projdata telos_projdata = { 0x544c4d53u /* "TLMS" */, CORE_PROJDATA_VARIABLE | sizeof pdata,
                                              &pdata, proj_loaded };

/* telos.c, after the boot's link: the index, and what runs. */
void telos_proj_boot(void)
{
    ix_update();
    run_build();
    boot_load = 1;
    boot_ticks = 0;
}

/* LOAD FOR THIS PROJECT, YES: those on, the set to check after the restart kept, and restart. */
static void load_and_restart(const u32 *pair, u32 n)
{
    u32 i;
    telos_dir_busy = 1;
    for (i = 0; i < pg[P_LOAD].n; i++)
        telos_dir.e[pg[P_LOAD].e[i]].flags |= TS_ON;
    if (telos_dir_write() != 0) {
        telos_dir_busy = 0;
        return;
    }
    telos_dir_busy = 0;
    ixh.pending = n < PEND_MAX ? n : PEND_MAX;
    for (i = 0; i < 2 * ixh.pending; i++)
        ixh.pend[i] = pair[i];
    ix_save();
    core_restart();
}


/* ---- the screens ---------------------------------------------------------------------------------- */
static const char *const titles[P_N] = { "LOAD FOR THIS PROJECT?", "NOT ON THIS UNIT", "CHANGED SINCE SAVE",
                                         "NOT IN THIS PROJECT" };
static const char *const feet[P_N] = { "YES LOAD+RESTART  NO SKIP", "YES OK", "YES CONTINUE  NO MODS",
                                       "YES OK" };

int telos_proj_key(void *brain, void *ev)
{
    uint32_t key = EV_KEY(ev), fl = EV_FLAGS(ev);
    (void)brain;
    if (page < 0) {
        if (eat && key == eat) {
            if (!(fl & 1))
                eat = 0;
            return 1;
        }
        return 0;
    }
    if (key == KEY_PLAY || key == KEY_STOP || key == KEY_TEMPO)
        return 0;
    if (!(fl & 1) || (fl & 8))
        return 1;                        /* releases and repeats */
    if (key != KEY_YES && key != KEY_NO)
        return 1;
    eat = (u8)key;
    if (page == P_LOAD && key == KEY_YES) {
        load_and_restart(last_pair, last_n);
    } else if (page == P_CHANGED && key == KEY_NO) {
        page = -1;
        dirty = 1;
        telos_page_open();
        return 1;
    }
    next_page();
    return 1;
}

void telos_proj_draw(void *bmp, void *ctrl)
{
    int i, y;
    (void)ctrl;
    if (page < 0)
        return;
    fw_fillrect(bmp, 0, 0, 127, 63, 0);
    TEXTF(bmp, fw_font_title, 2, 56, -1, "%s", titles[page]);
    fw_fillrect(bmp, 0, 54, 127, 63, -1);
    for (i = 0, y = 44; i < pg[page].n && i < 4; i++, y -= 10)
        TEXTF(bmp, fw_font5, 3, y, -1, "%s", pg[page].line[i]);
    if (pg[page].n > 4)
        TEXTF(bmp, fw_font5, 90, 14, -1, "+%d", pg[page].n - 4);
    TEXTF(bmp, fw_font5, 3, 2, -1, "%s", feet[page]);
}

void telos_proj_tick(void *ctrl)
{
    if (boot_load && ++boot_ticks > 300)
        boot_load = 0;                   /* no load at boot (a new unit): the next one is the user's */
    if (dirty) {
        dirty = 0;
        *((unsigned char *)ctrl + 0x20) = 1;
    }
}
