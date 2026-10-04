from geom import WEBOBJ, BYNAME, UNIQUE
from render_web import draw
from tiles import cells, canon
mx = 0
for name in UNIQUE:
    w = BYNAME[name]
    c = cells(draw(w))
    keys = set(canon(t)[0] for t in c.values())
    mx = max(mx, len(keys))
    print(name, len(c), len(keys))
print('max', mx)
