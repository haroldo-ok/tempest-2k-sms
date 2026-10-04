"""Pre-rendered, pre-scaled, pre-rotated sprite art (original geometric designs for this port).
Every image is a 16x16 box stored as four 8x8 4bpp tiles in 8x16-sprite order
(left column top/bottom, right column top/bottom).  Size index 0 is 'narrow' (8 wide)."""
import math, numpy as np
from PIL import Image, ImageDraw

SIZES = [7, 9, 11, 13, 15]
NANG = 16
# sprite palette indices
T, WHITE, YEL, ORG, RED, DRED, PUR, DPUR, GRN, DGRN, CYN, BLU, CYA, CYB, MAG, GRY = range(16)

def frame(a):
    th = a * 2 * math.pi / NANG
    n = (math.cos(th), math.sin(th))          # inward normal
    U = (-n[1], n[0]); V = (-n[0], -n[1])     # along-rim, outward
    return U, V

def xf(pts, cx, cy, r, a):
    U, V = frame(a)
    return [(cx + r * (x * U[0] + y * V[0]), cy + r * (x * U[1] + y * V[1])) for (x, y) in pts]

def new(sz):
    w = 8 if sz == 0 else 16
    return Image.new('P', (w, 16), 0), ((w - 1) / 2.0, 7.5)

def poly(d, pts, fill, outline):
    if fill: d.polygon(pts, fill=fill)
    if outline: d.line(pts + [pts[0]], fill=outline, width=1)

def pack(img):
    a = np.array(img, np.uint8)
    if a.shape[1] == 8: a = np.concatenate([a, np.zeros((16, 8), np.uint8)], 1)
    out = bytearray()
    for col in (0, 1):
        for row in (0, 1):
            t = a[row * 8:row * 8 + 8, col * 8:col * 8 + 8]
            out += tile4(t)
    return bytes(out)

def tile4(t):
    b = bytearray()
    for y in range(8):
        for p in range(4):
            v = 0
            for x in range(8):
                if (int(t[y, x]) >> p) & 1: v |= 0x80 >> x
            b.append(v)
    return bytes(b)

# ---- shape library (unit radius, y<0 = into the web) ----------------------
CLAW = [(-1, .55), (-.75, -.7), (-.35, .05), (0, -.35), (.35, .05), (.75, -.7), (1, .55), (0, .95)]
FLIP_L = [(-1, -.6), (-1, .6), (0, 0)]
FLIP_R = [(1, -.6), (1, .6), (0, 0)]
DIAM = [(0, -1), (1, 0), (0, 1), (-1, 0)]
CROWN = [(-1, .6), (-.9, -.8), (-.4, 0), (0, -1), (.4, 0), (.9, -.8), (1, .6)]
BAR = [(-1, -.3), (1, -.3), (1, .3), (-1, .3)]

def draw_kind(kind, a, anim, sz):
    img, (cx, cy) = new(sz); d = ImageDraw.Draw(img)
    r = SIZES[sz] / 2.0
    if kind == 'claw':
        poly(d, xf(CLAW, cx, cy, r, a), ORG, YEL)
    elif kind in ('flipper', 'mutant'):
        fc, oc = (DRED, RED) if kind == 'flipper' else (DPUR, RED)
        poly(d, xf(FLIP_L, cx, cy, r, a), fc, oc); poly(d, xf(FLIP_R, cx, cy, r, a), fc, oc)
    elif kind.startswith('tanker'):
        core = {'tanker': RED, 'tankerp': CYN, 'tankerf': CYA}[kind]
        poly(d, xf(DIAM, cx, cy, r, a), DPUR, PUR)
        poly(d, xf(DIAM, cx, cy, r * .42, a), core, core)
    elif kind == 'spiker':
        pts = []
        for i in range(25):
            t = i / 24.0; th = t * 3.4 * math.pi + anim * math.pi / 2
            rr = r * (.15 + .85 * t)
            pts.append((cx + rr * math.cos(th), cy + rr * math.sin(th)))
        d.line(pts, fill=GRN, width=1)
        d.point([(round(cx), round(cy))], fill=WHITE)
    elif kind == 'fuse':
        for k in range(5):
            th = k * 2 * math.pi / 5 + anim * math.pi / 5
            col = (CYA, CYB, WHITE, CYA, CYB)[k]
            mid = (cx + r * .55 * math.cos(th + .5), cy + r * .55 * math.sin(th + .5))
            end = (cx + r * math.cos(th), cy + r * math.sin(th))
            d.line([(cx, cy), mid, end], fill=col, width=1)
    elif kind == 'pulsar':
        col = CYN if anim == 0 else WHITE
        pts = [(-1 + i * .25, (.45 if i % 2 else -.45) if 0 < i < 8 else 0) for i in range(9)]
        d.line(xf(pts, cx, cy, r, a), fill=col, width=1)
        if anim: d.line(xf([(-1, 0), (1, 0)], cx, cy, r * .6, a), fill=BLU, width=1)
    elif kind == 'demon':
        poly(d, xf(CROWN, cx, cy, r, a), DRED, PUR)
    elif kind == 'mirror':
        poly(d, xf(BAR, cx, cy, r, a), GRY, WHITE)
    elif kind == 'ufo':
        e = [(cx + r * math.cos(t), cy + r * .45 * math.sin(t)) for t in np.linspace(0, 2 * math.pi, 14)[:-1]]
        poly(d, e, MAG, WHITE)
        d.ellipse([cx - r * .4, cy - r * .7, cx + r * .4, cy - r * .1], fill=CYN)
    elif kind == 'powerup':
        rr = r if anim == 0 else r * .8
        poly(d, [(cx, cy - rr), (cx + rr, cy), (cx, cy + rr), (cx - rr, cy)], None, CYN)
        q = r * (.45 if anim == 0 else .6)
        d.line([(cx - q, cy), (cx + q, cy)], fill=CYA); d.line([(cx, cy - q), (cx, cy + q)], fill=CYB)
    return img

# kind -> (n_angles, n_anim)
KINDS = [('claw', 16, 1), ('flipper', 16, 1), ('mutant', 16, 1), ('tanker', 16, 1), ('tankerp', 16, 1),
         ('tankerf', 16, 1), ('spiker', 1, 4), ('fuse', 1, 2), ('pulsar', 16, 2), ('demon', 16, 1),
         ('mirror', 16, 1), ('ufo', 1, 1), ('powerup', 1, 2)]

def build_images():
    imgs, base = [], {}
    for kind, na, nn in KINDS:
        base[kind] = len(imgs)
        for a in range(na):
            for an in range(nn):
                for sz in range(5):
                    imgs.append(pack(draw_kind(kind, a * (NANG // na) if na > 1 else 0, an, sz)))
    return imgs, base

# ---- static sprite tiles (8x16 pairs) -------------------------------------
def tall(draw_fn):
    img = Image.new('P', (8, 16), 0); d = ImageDraw.Draw(img); draw_fn(d); a = np.array(img, np.uint8)
    return tile4(a[0:8]) + tile4(a[8:16])
def big(draw_fn):
    img = Image.new('P', (16, 16), 0); d = ImageDraw.Draw(img); draw_fn(d); return pack(img)

def static_tiles():
    """returns dict name -> bytes (multiple of 64), in upload order"""
    S = {}
    # player shots: 3 sizes (diamonds)
    for i, r in enumerate((1, 2, 3)):
        S['shot%d' % i] = tall(lambda d, r=r: poly(d, [(3.5, 7.5 - r), (3.5 + r, 7.5), (3.5, 7.5 + r), (3.5 - r, 7.5)], WHITE, WHITE))
    S['spiketip'] = tall(lambda d: poly(d, [(3.5, 5), (6, 7.5), (3.5, 10), (1, 7.5)], WHITE, GRN))
    S['spikedot'] = tall(lambda d: d.rectangle([3, 6, 4, 9], fill=GRN))
    S['droid'] = tall(lambda d: poly(d, [(3.5, 4.5), (6.5, 7.5), (3.5, 10.5), (.5, 7.5)], None, CYB))
    # (the 'safe' block above lives at VRAM 436..447 and survives FMV playback)
    for i, r in enumerate((1, 2, 3)):
        S['laser%d' % i] = tall(lambda d, r=r: (d.line([(3.5 - r, 7.5), (3.5 + r, 7.5)], fill=YEL), d.line([(3.5, 7.5 - r - 1), (3.5, 7.5 + r + 1)], fill=ORG)))
    for i, r in enumerate((1, 2, 3)):
        S['eshot%d' % i] = tall(lambda d, r=r: (d.line([(3.5 - r, 7.5 - r), (3.5 + r, 7.5 + r)], fill=RED), d.line([(3.5 - r, 7.5 + r), (3.5 + r, 7.5 - r)], fill=WHITE)))
    for f in range(4):
        def ex(d, f=f):
            n = 8; cols = (WHITE, YEL, ORG, RED)
            for k in range(n):
                th = k * 2 * math.pi / n + f * .3
                r0, r1 = 1 + f * 1.6, 3 + f * 1.6
                d.line([(7.5 + r0 * math.cos(th), 7.5 + r0 * math.sin(th)), (7.5 + r1 * math.cos(th), 7.5 + r1 * math.sin(th))], fill=cols[f])
        S['expl%d' % f] = big(ex)
    return S

if __name__ == '__main__':
    imgs, base = build_images()
    print(len(imgs), 'images', len(imgs) * 128, 'bytes', base)
    # contact sheet for eyeballing
    pal = [(0,0,0),(255,255,255),(255,255,0),(255,170,0),(255,0,0),(170,0,0),(170,0,255),(85,0,170),(0,255,0),(0,170,0),(0,255,255),(0,85,255),(255,255,0),(255,85,255),(255,0,255),(170,170,170)]
    sheet = np.zeros((16 * 13, 16 * 16 * 5 // 2 * 2, 3), np.uint8)
    row = 0
    for kind, na, nn in KINDS:
        for a in range(min(na, 16)):
            img = np.array(draw_kind(kind, a, 0, 4 if a % 2 == 0 else 2))
            img = np.pad(img, ((0, 0), (0, 16 - img.shape[1])))
            sheet[row*16:row*16+16, a*16:a*16+16] = np.array(pal, np.uint8)[img]
        if na == 1:
            for an in range(nn):
                for sz in range(5):
                    img = np.array(draw_kind(kind, 0, an, sz)); img = np.pad(img, ((0, 0), (0, 16 - img.shape[1])))
                    sheet[row*16:row*16+16, (an*5+sz)*16:(an*5+sz)*16+16] = np.array(pal, np.uint8)[img]
        row += 1
    Image.fromarray(sheet).resize((sheet.shape[1]*3, sheet.shape[0]*3), Image.NEAREST).save('/tmp/t2ksms/gen/sprites_sheet.png')
