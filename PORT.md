# PORT.md — Dependency update for seven-ways

The game links against three local libraries that have been renamed and
relocated. This file documents the exact port from the old names/locations
to the new ones, plus every code change required.

## Dependency inventory

| Old name | Old location (sibling) | New name | System install | Build-tree copy |
|---|---|---|---|---|
| qmap | `../qmap` | corm | `/usr/lib/libcorm.so`, `/usr/include/ttypt/corm.h` | `../site/external/libcorm` |
| geo | `../geo` | islet | `/usr/lib/libislet.so`, `/usr/include/ttypt/islet.h` (+ `point.h`, `morton.h`, `pointcfg.h`) | `../site/external/libislet` |
| qsys | `../qsys` | qsys (unchanged) | `/usr/lib/libqsys.so`, `/usr/include/ttypt/qsys.h` | `../site/external/libqsys` |
| xxhash | system | (unchanged) | `/usr/lib/x86_64-linux-gnu/libxxhash.so` | — |
| qgl | `../qgl` | liblamina | `/usr/lib/liblamina.so`, `/usr/include/ttypt/lam*.h` | `../libqgl` |
| mk | `../mk` | mk (unchanged) | not installed | `../site/external/mk` |

Handy facts:

- `include/ttypt/qmap.h`, `geo.h`, `point.h` → now
  `include/ttypt/{corm,islet,point}.h` (corm and islet each ship their own tree; `point.h` comes with islet).
- `idm.h` and `queue.h` are shipped by libcorm; `dialog.c` already includes `<ttypt/idm.h>`.
- `ISLET_MISS == CM_MISS == QM_MISS == UINT32_MAX` (same value, so the sentinel swap is purely cosmetic but should still be done per-source).
- libislet is a thin layer over libcorm: `islet_open()` returns a plain corm map handle, so `corm_iter/corm_next/corm_drop` can be called straight on it (this is what `view.c` did on the old `smap_hd`).
- **corm, islet, qsys, xxhash and liblamina are installed in the system** (default
  compiler/linker search paths and module flag default), so the Makefile needs
  **no** `-I`/`-L` flags at all. `-llamina` and `<ttypt/lam*.h>` are resolved
  from `/usr/lib` and `/usr/include`.

---

## 1. `Makefile`

```make
# old
LDLIBS := -lm -lgeo -lxxhash -lqmap -lqsys -lqgl
LDLIBS += ${LDLIBS-${BE}}

LDLIBS-Linux += -lEGL

obj-y := game time
obj-y += tile char
obj-y += view map
obj-y += dialog
seven-ways-obj-y := ${obj-y:%=src/%.o}

tmxc-obj-y := map img png
tmxc-obj-y := ${tmxc-obj-y:%=src/%.o}
LDLIBS-tmxc := -ltmx -lz -lxml2

pngs :=  press seven tiles lamb rooster ui font
resources := ${pngs:%=resources/%.png} resources/icon.ico
map := col0 map0 map1 map2 map3
map := ${map:%=map/%.png} map/info.txt
share := map.txt ${resources} ${map}
share-dirs := resources map

CFLAGS := -g -I../qmap/include -I../geo/include -I../qgl/include

# Ensure the linker searches the updated local libraries first
LDFLAGS := -L../qgl -L../qmap -L../geo

-include ../mk/include.mk
```

```make
# new
LDLIBS := -lm -lislet -lxxhash -lcorm -lqsys -lqgl
LDLIBS += ${LDLIBS-${BE}}

LDLIBS-Linux += -lEGL

obj-y := game time
obj-y += tile char
obj-y += view map
obj-y += dialog
seven-ways-obj-y := ${obj-y:%=src/%.o}

tmxc-obj-y := map img png
tmxc-obj-y := ${tmxc-obj-y:%=src/%.o}
LDLIBS-tmxc := -ltmx -lz -lxml2

pngs :=  press seven tiles lamb rooster ui font
resources := ${pngs:%=resources/%.png} resources/icon.ico
map := col0 map0 map1 map2 map3
map := ${map:%=map/%.png} map/info.txt
share := map.txt ${resources} ${map}
share-dirs := resources map

CFLAGS := -g

-include ../site/external/mk/include.mk
```

Changes:

- `-lgeo` → `-lislet`, `-lqmap` → `-lcorm`, and `-lqgl` → `-llamina` (keep `-lm -lxxhash -lqsys`).
- **No `-I`/`-L` flags at all** — every library (corm, islet, qsys, xxhash, lamina)
  is installed in the system and found through the default search paths.
- The `include.mk` is now pulled from `../site/external/mk/include.mk` (the mk
  that libcorm/libislet themselves build with).
- Everything else in the Makefile (`BE`, `LDLIBS-${BE}`, `LDLIBS-Linux += -lEGL`,
  `obj-y`, `tmxc`, `pngs`, `map`, `share-dirs`) is unchanged.

Runtime note: every dependency resolves from `/usr/lib`, so no
`LD_LIBRARY_PATH` is needed to run the game.

---

## 2. `src/view.c`

Includes (lines 12-15):

```c
#include <ttypt/qsys.h>
#include <ttypt/qmap.h>   →  #include <ttypt/corm.h>
#include <ttypt/geo.h>    →  #include <ttypt/islet.h>
#include <ttypt/point.h>      /* unchanged (ships with islet) */
```

`dim` handling — the old runtime-dim geo API (`geo_*(hd, …, dim)`) is gone.
The new islet exposes per-dimension monomorphs plus a runtime-index table
`islet_ops[1..4]`, so the existing `dim` variable becomes an index into
`islet_ops`:

| Old | New |
|---|---|
| `geo_init()` | `islet_init()` |
| `geo_open(NULL, NULL, 0xFFFFF)` | `islet_open(NULL, NULL, 0xFFFFF)` |
| `geo_iter(smap_hd, tl, l, dim)` | `islet_ops[dim].iter(smap_hd, tl, l)` |
| `geo_next(p, &ref, cur)` | `islet_next(p, &ref, cur)` |
| `geo_put(smap_hd, s, ref, dim)` | `islet_ops[dim].put(smap_hd, s, ref)` |
| `geo_get(smap_hd, p, dim)` | `islet_ops[dim].get(smap_hd, p)` |
| `geo_del(smap_hd, p, dim)` | `islet_ops[dim].del(smap_hd, p)` |
| `qmap_drop(smap_hd)` | `corm_drop(smap_hd)` |
| `qmap_iter(smap_hd, NULL, 0)` | `corm_iter(smap_hd, NULL, 0)` |
| `qmap_next(&key, &value, cur)` | `corm_next(&key, &value, cur)` |
| `QM_MISS` (spatial context) | `ISLET_MISS` |

Exact call sites:

- `view_render()`: `geo_iter(smap_hd, tl, l, dim)` /
  `geo_next(p, &ref, cur)` → `islet_ops[dim].iter(...)` / `islet_next(...)`
- `vchar_put()`: `geo_put(smap_hd, s, ref, dim)` → `islet_ops[dim].put(...)`
- `view_init()`: `geo_init()`, `geo_open(...)`, `qmap_drop(smap_hd)` →
  `islet_init()`, `islet_open(...)`, `corm_drop(smap_hd)`
- `vchar_update()`: `geo_del(...)`, `geo_put(...)` → `islet_ops[dim].del(...)`,
  `islet_ops[dim].put(...)`
- `view_collides()`: `geo_get(...)`, `ret != QM_MISS`, `return QM_MISS` →
  `islet_ops[dim].get(...)`, `ret != ISLET_MISS`, `return ISLET_MISS`
- `view_update()`: `qmap_iter(...)`, `qmap_next(...)` → `corm_iter(...)`,
  `corm_next(...)`
- `vdialog_action()`: `npc == QM_MISS` → `npc == ISLET_MISS`

---

## 3. `src/char.c`

Includes: `#include <ttypt/qmap.h>` → `#include <ttypt/corm.h>`.

Simple prefix rename plus two constants:

| Old | New |
|---|---|
| `qmap_get(hd, key)` | `corm_get(hd, key)` |
| `qmap_put(hd, key, val)` | `corm_put(hd, key, val)` |
| `qmap_del(hd, key)` | `corm_del(hd, key)` |
| `qmap_reg(sizeof(char_t))` | `corm_reg(sizeof(char_t))` |
| `qmap_open(NULL, NULL, QM_HNDL, qm_char, 0xFF, QM_AINDEX)` | `corm_open(NULL, NULL, CM_HNDL, qm_char, 0xFF, CM_AINDEX)` |
| `QM_MISS` | `CM_MISS` |

Exact call sites:

- `char_render`, `char_face`, `char_animate`, `char_dir`, `char_animation`,
  `char_ipos`, `char_pos`, `char_update`, `char_dialog`, `char_talk`:
  `qmap_get` → `corm_get`
- `char_update`: `qmap_del(char_hd, &ref)` / `qmap_put(char_hd, &ref, &cho)`
  → `corm_del` / `corm_put`; also `!= QM_MISS` → `!= CM_MISS`
- `char_load`: `ch.dialog = QM_MISS;` → `CM_MISS`;
  `qmap_put(char_hd, NULL, &ch)` → `corm_put(...)` (NULL key + CM_AINDEX still
  returns the generated ref, exactly like the old `qmap_put`)
- `char_init`: `qmap_reg` → `corm_reg`, `qmap_open` → `corm_open` with
  `CM_HNDL`/`CM_AINDEX`

No semantic changes: with `CM_AINDEX`, `corm_put(hd, NULL, &v)` assigns and
returns the next auto key, identical to the old `QM_AINDEX` behavior.

---

## 4. `src/dialog.c`

Includes: `#include <ttypt/qmap.h>` → `#include <ttypt/corm.h>`.
`#include <ttypt/idm.h>` stays (now provided by libcorm).

Prefix rename + constant swap:

| Old | New |
|---|---|
| `qmap_get(...)` | `corm_get(...)` |
| `qmap_put(...)` | `corm_put(...)` |
| `qmap_reg(size)` | `corm_reg(size)` |
| `qmap_open(NULL, NULL, QM_HNDL, t, 0xFF, QM_AINDEX)` | `corm_open(NULL, NULL, CM_HNDL, t, 0xFF, CM_AINDEX)` |
| `QM_MISS` | `CM_MISS` |

- `update_options()`: `qmap_get(option_hd, &ref)` → `corm_get`
- `qui_rebuild()`: `cdialog.input != QM_MISS` → `!= CM_MISS`
- `dialog_begin()`: `qmap_get(dialog_hd, &ref)` → `corm_get`
- `dialog_init()`: three `qmap_reg` → `corm_reg`; three `qmap_open` →
  `corm_open` (with `CM_HNDL`/`CM_AINDEX`)
- `dialog_add()`: `d.input = QM_MISS` → `CM_MISS`; `qmap_put(dialog_hd, NULL, &d)`
  → `corm_put`
- `input_add()`: `qmap_put(input_hd, NULL, &d)` → `corm_put`
- `dialog_input`, `dialog_then`, `dialog_option`: `qmap_get(...)` → `corm_get`
- `dialog_option()`: `: QM_MISS` → `: CM_MISS`
- `dialog_action()`: three `qmap_get` → `corm_get`; `QM_MISS` → `CM_MISS`
- `input_press()`: `cdialog.input == QM_MISS` / `= QM_MISS` → `CM_MISS`;

The `ids_*` API (`ids_init`, `ids_push`, `ids_pop`, `ids_drop`, `ids_iter`,
`ids_next`) used by the dialog stack is unchanged — it still ships in
`<ttypt/idm.h>` from libcorm.

---

## 5. `src/tile.c`

`#include <ttypt/qmap.h>` (line 7) is **unused** — the file only uses qgl
drawing. Since `qmap.h` no longer exists, drop the include entirely so the
file no longer depends on corm. `#include <ttypt/qsys.h>` stays (provides
`UNUSED`).

---

## 6. `src/game.c`

`const uint8_t dim = 3;` — keep the value, but drop `const` so it matches the
`extern uint8_t dim;` declaration in `view.c` (a `const` definition vs. a
non-const `extern` across translation units is undefined behavior; worth
fixing while we are here since `dim` is now used as the `islet_ops[dim]`
index).

```c
uint8_t dim = 3;   /* was: const uint8_t dim = 3; */
```

---

## 7. `.github/workflows/ci.yml`

Package names changed; update the tty-pt deps:

```yaml
deps: "libislet,libqgl"
deps_openbsd: "libqsys,libislet,libqgl"
deps_winget_tty: "libqsys,libcorm,libislet,libqgl"
```

(`libgeo` → `libislet`, `libqmap` → `libcorm`. This mirrors the rename that
libcorm's and libislet's own READMEs document: "use **libcorm**/libislet as
the package name".)

---

## 8. Files intentionally not changed

- `info.txt` — historical project notes; still fine as a narrative. The
  `git@github.com:tty-pt/qmap.git` / `geo.git` repo URLs remain the upstream
  git remotes even though the local dirs are renamed.
- `README.md` — refers only to the `seven-ways` package; no lib names.
- `src/map.c`, `src/tmxc.c` and all headers under `include/` (except
  `include/time.h`, which the build blocker in §10 forced to be renamed to
  `include/gtime.h`) — no qmap/geo usage.

---

## 9. Verification

1. `make clean && make` — must compile the `seven-ways` binary with corm/islet
   resolved from the system install and link `-lislet -lcorm -lqsys -lqgl`.
   (Clean build also requires removing the stale `src/*.o.d` files first, since
   `make clean` does not delete them.)
2. Re-run `make` a second time to confirm no spurious rebuilds (rule
   stability). Verified.
3. Smoke-run the binary (headless it will log EGL/MESA driver warnings but
   still start; exit 0). No `LD_LIBRARY_PATH` needed:

```sh
./bin/seven-ways
```

4. If anything does not link (missing symbol), grep the new headers
   (`/usr/include/ttypt/corm.h`, `islet.h`) — every old `qmap_*`/`geo_*`
   symbol used here has a `corm_*`/`islet_*` counterpart.

---

## 10. Build blocker found and fixed during the port

- **`include/time.h` shadows `<time.h>`.** The game ships its own
  `include/time.h` (the clock module: `time_tick`, `time_init`, `dt_get`).
  mk adds `-I<project>/include` ahead of `/usr/include`, so the system
  qsys.h's own `#include <time.h>` resolved to the game header and
  `time_t`/`qsys_gmtime_r()` failed. Fix: renamed the project header to
  `include/gtime.h` and updated the six relative includes in
  `src/{dialog,game,char,seven-ways,time}.c`. All game headers are included
  via relative `../include/*.h` paths, so nothing else referenced it.
  Dedicated `libcorm/libislet/libqsys/libqgl` headers use `<ttypt/...>`
  angle includes and are unaffected.
- **qgl was not yet system-installed** during the first build attempt, so
  the Makefile temporarily carried `-I../libqgl/include` and
  `-L../libqgl/lib` (the shared lib lives in the repo's `lib/` subdir).
  After `libqgl` was installed system-wide, those flags were removed.
- **Pre-existing, left alone:** `include/view.h` declares
  `int view_collides(...)` while `src/view.c` defines `unsigned
  view_collides(...)`; `char_update()`'s `!= CM_MISS` comparison still
  warns `-Wsign-compare`. Not introduced by this port.