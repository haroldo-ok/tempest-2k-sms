"""Build all offline data for the SMS port: gen/blob.bin (ROM banks 2..) + gen/assets.h/.c"""
import sys, math, struct, pickle, numpy as np
from geom import WEBOBJ, BYNAME, UNIQUE, DEPTH
from render_web import draw
from tiles import cells, canon
from fmvenc import Dict, frame_map, VState, encode_frame, converge, canonical_state
import webtables, sprites, font
from blob import Blob, BANK
from raster import canvas

FONT_BASE = 1
SLOT_FMV = list(range(41, 432))
SLOT_CANON = list(range(41, 192))
LIT_POOL = (192, 224)        # two pools of 32
N_WARP = 40; N_ZOOM = 40; N_TITLE = 64
QUICK = '--quick' in sys.argv
import os
OUT = sys.argv[sys.argv.index('--out') + 1] if '--out' in sys.argv else '../gen'
os.makedirs(OUT, exist_ok=True)
webs_to_do = UNIQUE[:3] if QUICK else UNIQUE

D = Dict()
def ent(slot, fl): return slot | ((fl & 1) << 9) | ((fl >> 1) << 10)

def enc_step(step, flags, hold=0, aux=0):
    ups, nt = step
    b = bytearray([flags, hold, aux]) + struct.pack('<H', len(ups))
    for s, k in ups:
        b += bytes([s & 0xFF, (s >> 8) | ((k >> 16) << 1)]) + struct.pack('<H', k & 0xFFFF)
    cellsn = sorted(nt)
    runs = []
    for c in cellsn:
        e = 0 if nt[c] is None else ent(*nt[c])
        if runs and runs[-1][0] + len(runs[-1][1]) == c and len(runs[-1][1]) < 255: runs[-1][1].append(e)
        else: runs.append([c, [e]])
    b += struct.pack('<H', len(runs))
    for c, es in runs:
        b += struct.pack('<HB', c, len(es))
        for e in es: b += struct.pack('<H', e)
    return bytes(b)

# ------------------------------------------------------------------ clips
def warp_frames(w):
    out = []
    for k in range(N_WARP):
        t = k / (N_WARP - 1); e = (1 - t) ** 2.2
        out.append(draw(w, cam_z=44.0 * e, rot=math.pi * e))
    return out
def zoom_frames(w):
    out, aux = [], []
    for k in range(N_ZOOM):
        t = k / (N_ZOOM - 1); cz = -24.0 * (t ** 1.6)
        out.append(draw(w, cam_z=cz)); aux.append(int(round(-cz * 4)))
    return out, aux

def text_px(img, s, x, y, c, scale=1):
    for i, ch in enumerate(s):
        if ch == ' ': continue
        g = font.glyph_px(ch)
        for yy in range(8):
            for xx in range(8):
                if g[yy, xx]:
                    img[y + yy * scale:y + yy * scale + scale, x + (i * 8 + xx) * scale:x + (i * 8 + xx) * scale + scale] = c
def title_frames():
    w = BYNAME['web5']
    out = []
    for k in range(N_TITLE):
        img = draw(w, cam_z=12.0, rot=2 * math.pi * k / N_TITLE)
        text_px(img, 'TEMPEST 2K', 128 - 10 * 16 // 2 - 4, 18, 3, 2)
        text_px(img, 'PRESS BUTTON 1', 128 - 14 * 4, 176, 3)
        out.append(img)
    return out

blob = Blob()
# ---- static resources first (bank 2): font + static sprite tiles + palettes
res = bytearray()
fontdata = bytearray()
for ch in font.ORDER:
    g = font.glyph_px(ch) * 15
    fontdata += sprites.tile4(g)
static = sprites.static_tiles()
SAFE_ORDER = ['shot0', 'shot1', 'shot2', 'spiketip', 'spikedot', 'droid']
OTHER_ORDER = ['laser0', 'laser1', 'laser2', 'eshot0', 'eshot1', 'eshot2', 'expl0', 'expl1', 'expl2', 'expl3']
safe = b''.join(static[k] for k in SAFE_ORDER)
other = b''.join(static[k] for k in OTHER_ORDER)
font_loc = blob.put(bytes(fontdata), new_bank=True)
safe_loc = blob.put(safe)
other_loc = blob.put(other)
blank_title_loc = None

# ---- sprite images
imgs, imgbase = sprites.build_images()
img_bank0 = None
for i in range(0, len(imgs), 128):
    loc = blob.put(b''.join(imgs[i:i + 128]), new_bank=True)
    if img_bank0 is None: img_bank0 = loc[0]
    assert loc[1] == 0

# ---- web tables (2 per bank)
web_loc = {}
for i, name in enumerate(UNIQUE):
    w = BYNAME[name]
    t = webtables.build(w)
    if i % 2 == 0: b = blob.new_bank(); blob.banks[b] += bytes(2 * 8192)
    rb = blob.first + len(blob.banks) - 1
    off = (i % 2) * 8192
    blob.at(rb, off, t)
    web_loc[name] = (rb, off)

# ---- per web: lit overlays, canonical snapshot, warp-in + zoom-out clips
maxlit = 0
stats = []
for name in webs_to_do:
    w = BYNAME[name]
    base = draw(w)
    fm = frame_map(base, D)
    cs = canonical_state(SLOT_CANON, fm)
    # canonical snapshot step (applied with display off / blank screen)
    v0 = VState(SLOT_CANON)
    canon_step = (sorted(((s, k) for s, k in cs.content.items())), {c: e for c, e in cs.nt.items()})
    canon_bytes = enc_step(canon_step, 3)
    # lit overlays
    lit_index = bytearray()
    bc = cells(base)
    for lane in range(w.lanes):
        li = draw(w, lit=lane)
        lc = cells(li)
        diff = sorted(c for c in set(lc) | set(bc) if c not in bc or c not in lc or not np.array_equal(lc[c], bc[c]))
        diff = [c for c in diff if c in lc]
        maxlit = max(maxlit, len(diff))
        assert len(diff) <= 32, (name, lane, len(diff))
        rec = bytearray([len(diff)])
        for c in diff:
            be = ent(*cs.nt[c]) if c in cs.nt else 0
            rec += struct.pack('<HH', 0x3800 + c * 2, be)
        for c in diff:
            t = lc[c]
            for y in range(8):
                for p in range(3):
                    v = 0
                    for x in range(8):
                        if (int(t[y, x]) >> p) & 1: v |= 0x80 >> x
                    rec.append(v)
        loc = blob.put(bytes(rec))
        lit_index += bytes([loc[0]]) + struct.pack('<H', 0x8000 + loc[1])
    canon_loc = blob.put(canon_bytes)
    # warp-in
    fr = [frame_map(i, D) for i in warp_frames(w)]
    v = VState(SLOT_FMV)
    chunks = []
    for k, f in enumerate(fr):
        chunks.append(enc_step(encode_frame(v, f), 1, 2, 0))
    conv = converge(v, cs)
    for k, st in enumerate(conv):
        chunks.append(enc_step(st, 2 if k == len(conv) - 1 else 0))
    warp_loc = blob.put_stream(chunks)
    stats.append(('warp', name, sum(len(c) for c in chunks)))
    # zoom-out (starts from canonical, unlit)
    zf, aux = zoom_frames(w)
    v = cs.copy(); v.slots = SLOT_FMV
    chunks = []
    for k, img in enumerate(zf):
        st = encode_frame(v, frame_map(img, D))
        chunks.append(enc_step(st, 1 | (2 if k == len(zf) - 1 else 0), 2, aux[k]))
    zoom_loc = blob.put_stream(chunks)
    stats.append(('zoom', name, sum(len(c) for c in chunks)))
    # write per-web pointers into the table slot (offset 0x1700)
    rb, off = web_loc[name]
    ptr = bytearray(lit_index) + bytes(3 * (18 - w.lanes))
    for loc in (canon_loc, warp_loc, zoom_loc):
        ptr += bytes([loc[0]]) + struct.pack('<H', 0x8000 + loc[1])
    blob.at(rb, off + 0x1700, ptr)
    print(name, 'lit', maxlit, 'dict', len(D.tiles), 'banks', len(blob.banks), flush=True)

# ---- title clip (intro + loop body with converge)
tf = [frame_map(i, D) for i in title_frames()]
v = VState(SLOT_FMV)
intro = [enc_step(encode_frame(v, f), 1, 3) for f in tf]
ref = v.referenced(); v.content = {s: k for s, k in v.content.items() if s in ref}
target = v.copy()
body = [enc_step(encode_frame(v, f), 1, 3) for f in tf]
conv = converge(v, target)
body += [enc_step(st, 0) for st in conv]
body[-1] = bytes([body[-1][0] | 2]) + body[-1][1:]
title_loc = blob.put_stream(intro)
title_loop_loc = blob.put_stream(body)
# NB: put_stream keeps going in the same bank sequence, so intro runs straight into body

# ---- tile dictionary (2bpp, 1024 per bank)
def pack2(key):
    a = np.frombuffer(key, np.uint8).reshape(8, 8)
    b = bytearray()
    for y in range(8):
        for p in range(2):
            v = 0
            for x in range(8):
                if (int(a[y, x]) >> p) & 1: v |= 0x80 >> x
            b.append(v)
    return bytes(b)
dict_bank0 = None
for i in range(0, len(D.tiles), 1024):
    loc = blob.put(b''.join(pack2(k) for k in D.tiles[i:i + 1024]), new_bank=True)
    if dict_bank0 is None: dict_bank0 = loc[0]
nbanks = len(blob.banks) + 2
print('total banks', nbanks, 'dict tiles', len(D.tiles), 'max lit', maxlit)
assert nbanks <= 256, nbanks
assert len(D.tiles) <= 65536
open(OUT + '/blob.bin', 'wb').write(blob.bytes())

# ---- C index
lvl_web = [UNIQUE.index(w.name) for w in WEBOBJ]
h = open(OUT + '/assets.h', 'w')
h.write('/* generated by tools/build.py */\n#ifndef ASSETS_H\n#define ASSETS_H\n')
h.write('#define FONT_BANK %d\n#define FONT_ADDR 0x%04X\n' % (font_loc[0], 0x8000 + font_loc[1]))
h.write('#define FONT_COUNT %d\n' % len(font.ORDER))
h.write('#define SAFE_BANK %d\n#define SAFE_ADDR 0x%04X\n#define SAFE_SIZE %d\n' % (safe_loc[0], 0x8000 + safe_loc[1], len(safe)))
h.write('#define OTHER_BANK %d\n#define OTHER_ADDR 0x%04X\n#define OTHER_SIZE %d\n' % (other_loc[0], 0x8000 + other_loc[1], len(other)))
h.write('#define IMG_BANK0 %d\n#define DICT_BANK0 %d\n' % (img_bank0, dict_bank0))
for k, v in imgbase.items(): h.write('#define IMG_%s %d\n' % (k.upper(), v))
h.write('#define TITLE_BANK %d\n#define TITLE_ADDR 0x%04X\n' % (title_loc[0], 0x8000 + title_loc[1]))
h.write('#define TITLE_LOOP_BANK %d\n#define TITLE_LOOP_ADDR 0x%04X\n' % (title_loop_loc[0], 0x8000 + title_loop_loc[1]))
h.write('#define NUM_UWEBS %d\n#define NUM_LEVEL_WEBS 48\n' % len(UNIQUE))
h.write('extern const unsigned char level_web[48];\nextern const unsigned char uweb_bank[%d];\nextern const unsigned int uweb_addr[%d];\n' % (len(UNIQUE), len(UNIQUE)))
h.write('#endif\n')
c = open(OUT + '/assets.c', 'w')
c.write('#include "assets.h"\n')
c.write('const unsigned char level_web[48] = {%s};\n' % ','.join(map(str, lvl_web)))
c.write('const unsigned char uweb_bank[%d] = {%s};\n' % (len(UNIQUE), ','.join(str(web_loc[n][0]) for n in UNIQUE)))
c.write('const unsigned int uweb_addr[%d] = {%s};\n' % (len(UNIQUE), ','.join('0x%04X' % (0x8000 + web_loc[n][1]) for n in UNIQUE)))
pickle.dump(dict(D=D.tiles, stats=stats), open(OUT + '/build.pkl', 'wb'))
for s in stats[:6]: print(s)
