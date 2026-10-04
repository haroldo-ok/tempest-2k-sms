import math
from geom import BYNAME
from render_web import draw
import font
N_WARP = 40; N_ZOOM = 40; N_TITLE = 64
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

