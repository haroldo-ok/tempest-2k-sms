"""Replay every FMV clip from the ROM image exactly as fmv.c does and compare the
reconstructed screen against the ideal offline renders (must be pixel-exact)."""
import sys, struct, re, numpy as np
sys.argv += []
from geom import BYNAME, UNIQUE
import build_frames as BF
rom = open('../rom/t2k_sms.sms', 'rb').read()
H = open('../gen/assets.h').read()
def d(name): return int(re.search(r'#define %s (\w+)' % name, H).group(1), 0)
DICT0 = d('DICT_BANK0')
def rd(bank, addr, n): o = bank * 16384 + (addr - 0x8000); return rom[o:o + n]
def dict_tile(tid):
    raw = rd(DICT0 + (tid >> 10), 0x8000 + (tid & 1023) * 16, 16)
    t = np.zeros((8, 8), np.uint8)
    for y in range(8):
        for p in range(2):
            b = raw[y * 2 + p]
            for x in range(8):
                if b & (0x80 >> x): t[y, x] |= 1 << p
    return t
class Player:
    def __init__(self): self.vram = {}; self.nt = [0] * 768
    def screen(self):
        img = np.zeros((192, 256), np.uint8)
        for c, e in enumerate(self.nt):
            if c < 32: continue
            s = e & 511
            t = self.vram.get(s, np.zeros((8, 8), np.uint8)) if s else np.zeros((8, 8), np.uint8)
            if e & 0x200: t = t[:, ::-1]
            if e & 0x400: t = t[::-1, :]
            y, x = divmod(c, 32); img[y*8:y*8+8, x*8:x*8+8] = t
        return img
    def play(self, bank, addr, maxsteps=100000):
        """yields (flags, aux, screen) after each visible step"""
        o = bank * 16384 + addr - 0x8000
        for _ in range(maxsteps):
            if rom[o] == 0xFF: bank += 1; o = bank * 16384
            fl, hold, aux, nup = rom[o], rom[o+1], rom[o+2], struct.unpack_from('<H', rom, o + 3)[0]; o += 5
            for k in range(nup):
                s = rom[o] | ((rom[o+1] & 1) << 8); tid = struct.unpack_from('<H', rom, o + 2)[0]; o += 4
                assert 41 <= s < 432, s
                self.vram[s] = dict_tile(tid)
            nr = struct.unpack_from('<H', rom, o)[0]; o += 2
            for r in range(nr):
                c, n = struct.unpack_from('<HB', rom, o); o += 3
                for k in range(n):
                    self.nt[c + k] = struct.unpack_from('<H', rom, o)[0]; o += 2
            if fl & 1: yield fl, aux, self.screen()
            if fl & 2: yield 'end', aux, self.screen(); return
def check_clip(bank, addr, frames, pl, name, final=None):
    k = 0; bad = 0
    for fl, aux, scr in pl.play(bank, addr):
        if fl == 'end':
            if final is not None and not np.array_equal(scr[8:], final[8:]): print(name, 'FINAL MISMATCH'); bad += 1
            break
        if k < len(frames) and not np.array_equal(scr[8:], frames[k][8:]):
            bad += 1; print(name, 'frame', k, 'diff px', int((scr[8:] != frames[k][8:]).sum()))
        k += 1
    assert k == len(frames), (name, k, len(frames))
    return bad

C = open('../gen/assets.c').read()
ub = [int(x) for x in re.search(r'uweb_bank\[\d+\] = \{([^}]*)\}', C).group(1).split(',')]
ua = [int(x, 16) for x in re.search(r'uweb_addr\[\d+\] = \{([^}]*)\}', C).group(1).split(',')]
from render_web import draw
tot = 0
only = sys.argv[1:] 
for i, name in enumerate(UNIQUE):
    if only and name not in only: continue
    w = BYNAME[name]
    ptr = rd(ub[i], ua[i] + 0x1700 + 54, 9)
    canon = (ptr[0], ptr[1] | ptr[2] << 8); warp = (ptr[3], ptr[4] | ptr[5] << 8); zoom = (ptr[6], ptr[7] | ptr[8] << 8)
    base = draw(w)
    pl = Player()
    for _ in pl.play(*canon): pass
    b = int(not np.array_equal(pl.screen()[8:], base[8:]))
    canon_nt = list(pl.nt); canon_vram = dict(pl.vram)
    pl = Player()
    b += check_clip(*warp, BF.warp_frames(w), pl, name + ' warp', final=base)
    # after warp the layout must equal the canonical one (lit overlays rely on it)
    if pl.nt != canon_nt: print(name, 'warp end layout != canonical'); b += 1
    zf, aux = BF.zoom_frames(w)
    b += check_clip(*zoom, zf, pl, name + ' zoom')
    print(name, 'OK' if not b else 'BAD %d' % b, flush=True)
    tot += b
pl = Player()
tf = BF.title_frames()
b = check_clip(d('TITLE_BANK'), d('TITLE_ADDR'), tf + tf, pl, 'title intro+loop')
b += check_clip(d('TITLE_LOOP_BANK'), d('TITLE_LOOP_ADDR'), tf, pl, 'title loop 1')
b += check_clip(d('TITLE_LOOP_BANK'), d('TITLE_LOOP_ADDR'), tf, pl, 'title loop 2')
print('title', 'OK' if not b else 'BAD'); tot += b
print('TOTAL MISMATCHES', tot)
