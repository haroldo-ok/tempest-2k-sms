import sys
from fmvenc import *
from clips import *
from geom import BYNAME, UNIQUE
D = Dict()
SLOTS = list(range(41, 432)); CANON = list(range(41, 192))
tot_up = 0; tot_nt = 0; nsteps = 0; worst = 0
for name in UNIQUE[:int(sys.argv[1])]:
    w = BYNAME[name]
    fr = [frame_map(i, D) for i in warp_in(w)]
    v = VState(SLOTS); steps = [encode_frame(v, f) for f in fr]
    cs = canonical_state(CANON, fr[-1]); steps += converge(v, cs)
    zo = [frame_map(i, D) for i in zoom_out(w)]
    v = cs.copy(); v.slots = SLOTS; steps2 = [encode_frame(v, f) for f in zo]
    for st in steps + steps2:
        u, n = step_bytes(st); tot_up += u; tot_nt += n; nsteps += 1; worst = max(worst, u + n)
    print(name, 'warp', [len(s[0]) for s in steps], 'zoom', [len(s[0]) for s in steps2])
print('dict', len(D.tiles), 'steps', nsteps, 'avg up', tot_up / nsteps, 'avg nt', tot_nt / nsteps, 'worst', worst)
