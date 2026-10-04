import math
from geom import BYNAME, UNIQUE
from render_web import draw
def warp_in(w, n=40):
    out = []
    for k in range(n):
        t = k / (n - 1)
        e = (1 - t) ** 2.2
        out.append(draw(w, cam_z=44.0 * e, rot=math.pi * 1.0 * e))
    return out
def zoom_out(w, n=40):
    out = []
    for k in range(n):
        t = k / (n - 1)
        cz = -24.0 * (t ** 1.6)
        out.append(draw(w, cam_z=cz))
    return out
def title(n=64):
    w = BYNAME['web5']
    return [draw(w, cam_z=6.0, rot=2 * math.pi * k / n) for k in range(n)]
