"""8x8 tile cutting + flip-aware canonicalisation."""
import numpy as np
def cells(img, row0=1):
    H, W = img.shape
    out = {}
    for cy in range(row0, H // 8):
        for cx in range(W // 8):
            t = img[cy*8:cy*8+8, cx*8:cx*8+8]
            if t.any(): out[cy * 32 + cx] = t
    return out
def canon(t):
    """return (key bytes, flipflags) where key is the canonical orientation; flags = which flip of key gives t"""
    cands = [(t.tobytes(), 0), (t[:, ::-1].tobytes(), 1), (t[::-1, :].tobytes(), 2), (t[::-1, ::-1].tobytes(), 3)]
    k = min(cands)
    return k[0], k[1]   # t == flip(k, flags)
