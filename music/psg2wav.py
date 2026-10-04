"""Render a smstest psglog (frame, byte) into a WAV (SN76489 model, 8x oversampled)."""
import sys, wave, numpy as np
CLOCK = 3579545.0; RATE = 44100; OS = 8; SPF = RATE / 60.0
VOL = np.array([10 ** (-0.1 * i) for i in range(15)] + [0.0])
def render(path, out, f0=None, f1=None):
    ev = {}
    for ln in open(path):
        f, b = ln.split(); ev.setdefault(int(f), []).append(int(b, 16))
    frames = sorted(ev); f0 = frames[0] if f0 is None else f0; f1 = frames[-1] + 30 if f1 is None else f1
    per = [1, 1, 1]; att = [15] * 4; nmode = 0; latch = 0
    ph = np.zeros(3); lfsr = 0x8000; nph = 0.0
    chunks = []
    for f in range(f0, f1):
        for b in ev.get(f, []):
            if b & 0x80:
                latch = (b >> 4) & 7; ch = latch >> 1
                if latch & 1: att[ch] = b & 15
                elif ch < 3: per[ch] = (per[ch] & 0x3F0) | (b & 15)
                else: nmode = b & 7; lfsr = 0x8000
            else:
                ch = latch >> 1
                if latch & 1: att[ch] = b & 15
                elif ch < 3: per[ch] = (per[ch] & 15) | ((b & 0x3F) << 4)
        n = (int(SPF * (f - f0 + 1)) - int(SPF * (f - f0))) * OS
        s = np.zeros(n)
        k = np.arange(1, n + 1)
        for c in range(3):
            p = max(per[c], 1); inc = CLOCK / 32.0 / p / (RATE * OS)     # cycles per sample
            phase = ph[c] + k * inc; ph[c] = phase[-1] % 1.0
            if p > 6 and att[c] < 15: s += np.where((phase % 1.0) < 0.5, 1.0, -1.0) * VOL[att[c]]
        if att[3] < 15:
            div = (16, 32, 64)[nmode & 3] if (nmode & 3) < 3 else max(per[2], 1)
            inc = CLOCK / 16.0 / div / (RATE * OS)
            pos = nph + k * inc; idx = np.floor(pos).astype(int); nph = pos[-1] % 1.0
            nshift = idx[-1]; bits = np.empty(nshift + 1)
            bits[0] = lfsr & 1
            for i in range(1, nshift + 1):
                fb = ((lfsr & 1) ^ ((lfsr >> 3) & 1)) if nmode & 4 else (lfsr & 1)
                lfsr = (lfsr >> 1) | (fb << 15); bits[i] = lfsr & 1
            s += (bits[idx] * 2 - 1) * VOL[att[3]]
        chunks.append(s.reshape(-1, OS).mean(1))
    a = np.concatenate(chunks) * 32767 * 0.22
    a = np.clip(a, -32767, 32767).astype(np.int16)
    w = wave.open(out, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(a.tobytes()); w.close()
    print(out, round(len(a) / RATE, 1), 's')
if __name__ == '__main__': render(sys.argv[1], sys.argv[2])
