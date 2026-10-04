"""Original music for the SMS port (composed for this port; not derived from any existing tune).
Each bar = 16 rows (16th notes).  Tokens: note (A4, C#5, Bb3), '.' hold, '-' off,
arp chords on the pad channel: note + 'm' (minor) / 'M' (major), drums: K S h o ('.' rest)."""

def bass8(r1, r2):           # octave-bouncing eighths
    return f"{r1} . {r2} . {r1} . {r2} . {r1} . {r2} . {r1} . {r2} ."
def bass16(r1, r2):          # driving sixteenths with accents
    return f"{r1} . {r1} {r2} . {r1} {r1} . {r1} . {r1} {r2} . {r1} {r2} {r1}"
def bassB(r1, r2):
    return f"{r1} . . {r1} {r1} . {r2} . {r1} . . {r1} {r1} . {r2} {r1}"
def pad(ch):                 # chord held for the bar
    return ch + " . . . . . . . . . . . . . . ."
REST = "- . . . . . . . . . . . . . . ."

SONGS = {}

# ---- 0: title  "Vector Dawn"  (A minor, 112 bpm-ish) -------------------------
prog = [("A2", "A3", "A4m"), ("F3", "F2" if False else "C3", "F4M"), ("C3", "G3", "C4M"), ("G2" if False else "B2", "G3", "G4M"),
        ("A2", "A3", "A4m"), ("F3", "C3", "F4M"), ("G3", "D3", "G4M"), ("E3", "B2", "E4M")]
lead = [
    "E5 . . . A4 . . . C5 . B4 . A4 . E4 .",
    "F4 . . . A4 . C5 . F5 . . . E5 . C5 .",
    "G4 . . . C5 . E5 . G5 . . . F5 . E5 .",
    "D5 . . . B4 . G4 . D5 . E5 . D5 . B4 .",
    "A4 . C5 . E5 . A5 . G5 . E5 . C5 . E5 .",
    "F5 . . . E5 . C5 . A4 . C5 . F5 . . .",
    "G5 . F5 . D5 . B4 . G4 . B4 . D5 . F5 .",
    "G#5 . . . . . . . E5 . . . B4 . G#4 .",
]
SONGS['title'] = dict(tempo=8, loop=0, ins=('pluck', 'lead', 'pad'),
    ch0=[bass8(a, b) for a, b, c in prog], ch1=lead, ch2=[pad(c) for a, b, c in prog],
    noise=["K . h . S . h . K . h K S . h o"] * 7 + ["K . h . S . h . K . S . S S S S"])

# ---- 1: level tune A  "Lane Runner"  (D minor, fast) -------------------------
prog = [("D3", "D4", "D4m"), ("A#2", "A#3", "A#3M"), ("C3", "C4", "C4M"), ("A2", "A3", "A3m")]
leadA = [
    "D5 . . A4 . . D5 . F5 . E5 . D5 . C5 .",
    "D5 . . . . . F5 . . . D5 . A#4 . . .",
    "E5 . . C5 . . G5 . . . E5 . C5 . D5 .",
    "E5 . . . . . A4 . C5 . E5 . A5 . . .",
]
SONGS['levelA'] = dict(tempo=6, loop=4, ins=('pluck', 'lead', 'pad'),
    ch0=[bass16(a, b) for a, b, c in prog] * 3, ch1=[REST] * 4 + leadA + [l.replace('5', '6').replace('A4', 'A5') for l in leadA],
    ch2=[pad(c) for a, b, c in prog] * 3,
    noise=["K . h . K . h . K . h . K . h h"] * 3 + ["K . h . K . h . K . S . S S S S"] +
          ["K . h . S . h . K . h K S . h o"] * 7 + ["K . S . S . S . S S S S S S S S"])

# ---- 2: level tune B  "Grid Pulse"  (E minor) --------------------------------
prog = [("E3", "B2", "E4m"), ("C3", "G3", "C4M"), ("D3", "A3", "D4M"), ("B2", "F#3", "B3m")]
leadB = [
    "B4 . E5 . G5 . . . F#5 . E5 . D5 . B4 .",
    "C5 . . . E5 . G5 . . . E5 . C5 . . .",
    "D5 . F#5 . A5 . . . G5 . F#5 . E5 . D5 .",
    "F#5 . . . . . D5 . B4 . . . F#4 . . .",
    "E5 . . E5 . . G5 . B5 . A5 . G5 . E5 .",
    "G5 . . . E5 . C5 . E5 . . . G5 . . .",
    "A5 . . . F#5 . D5 . F#5 . A5 . B5 . A5 .",
    "F#5 . . . D5 . . . B4 . . . - . . .",
]
SONGS['levelB'] = dict(tempo=6, loop=0, ins=('pluck', 'lead', 'pad'),
    ch0=[bassB(a, b) for a, b, c in prog] * 2, ch1=leadB, ch2=[pad(c) for a, b, c in prog] * 2,
    noise=["K . h K S . h . K . h . S . h h"] * 7 + ["K . h K S . S . S S S . S S S S"])

# ---- 3: level tune C  "Spike Field"  (G minor) -------------------------------
prog = [("G3", "D3", "G4m"), ("D#3", "A#2", "D#4M"), ("F3", "C3", "F4M"), ("D3", "A2", "D4M")]
leadC = [
    "G4 . A#4 . D5 . G5 . F5 . D5 . A#4 . D5 .",
    "D#5 . . . G5 . . . A#5 . . . G5 . D#5 .",
    "F5 . . . C5 . A4 . C5 . F5 . A5 . F5 .",
    "F#5 . . . . . A5 . . . F#5 . D5 . . .",
]
SONGS['levelC'] = dict(tempo=6, loop=0, ins=('pluck', 'lead', 'pad'),
    ch0=[bass16(a, b) for a, b, c in prog] * 2, ch1=leadC + [REST, REST, REST, "F#5 . . . A5 . . . D6 . . . - . . ."],
    ch2=[pad(c) for a, b, c in prog] * 2,
    noise=["K . h . S . h K . K h . S . h o"] * 7 + ["K . S . K . S . S S S . S S S S"])

# ---- 4: game over sting --------------------------------------------------------
SONGS['gameover'] = dict(tempo=9, loop=None, ins=('pluck', 'lead', 'pad'),
    ch0=["A2 . . . . . . . F3 . . . . . . .", "E3 . . . . . . . A2 . . . . . . -"],
    ch1=["E5 . . . C5 . . . A4 . . . E4 . . .", "G#4 . . . . . . . A4 . . . . . . -"],
    ch2=["A4m . . . . . . . F4M . . . . . . .", "E4M . . . . . . . A3m . . . . . . -"],
    noise=["K . . . . . . . K . . . . . . .", "S . . . . . . . K . . . . . . ."])

# ---- 5: warp-in fanfare (plays over the fly-in) -------------------------------
SONGS['warp'] = dict(tempo=4, loop=None, ins=('pluck', 'lead', 'pad'),
    ch0=["A2 . . . A3 . . . A2 . . . A3 . . .", "E3 . . . E3 . . . A3 . . . . . . -"],
    ch1=["A4 . C5 . E5 . A5 . C6 . E6 . A6 . . .", "G#6 . . . E6 . . . A6 . . . . . . -"],
    ch2=["A4m . . . . . . . . . . . . . . .", "E4M . . . . . . . A4m . . . . . . -"],
    noise=["K . h . S . h . K . h . S . S S", "K . S S S S S S K . . . . . . ."])

ORDER = ['title', 'levelA', 'levelB', 'levelC', 'gameover', 'warp']
