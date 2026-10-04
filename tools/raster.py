"""Tiny indexed-colour rasteriser for offline frames (numpy)."""
import numpy as np, math
W, H = 256, 192
def canvas(): return np.zeros((H, W), np.uint8)
def plot(img, x, y, c):
    if 0 <= x < W and 0 <= y < H: img[y, x] = c
def line(img, x0, y0, x1, y1, c, fat=False, clip=None):
    # clip to a generous box to keep long lines cheap
    dx, dy = x1 - x0, y1 - y0
    n = int(max(abs(dx), abs(dy))) + 1
    if n > 4000:   # clip parametric to screen box
        ts = []
        for t in np.linspace(0, 1, 20001):
            x, y = x0 + dx * t, y0 + dy * t
            if -4 <= x <= W + 4 and -4 <= y <= H + 4: ts.append(t)
        if not ts: return
        t0, t1 = ts[0], ts[-1]
        x0, y0, x1, y1 = x0 + dx * t0, y0 + dy * t0, x0 + dx * t1, y0 + dy * t1
        dx, dy = x1 - x0, y1 - y0
        n = int(max(abs(dx), abs(dy))) + 1
    for i in range(n + 1):
        t = i / n if n else 0
        x = int(math.floor(x0 + dx * t + 0.5)); y = int(math.floor(y0 + dy * t + 0.5))
        if clip and not clip(x, y): continue
        plot(img, x, y, c)
        if fat:
            if abs(dx) > abs(dy): plot(img, x, y + 1, c)
            else: plot(img, x + 1, y, c)
