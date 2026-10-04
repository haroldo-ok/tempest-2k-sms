BANK = 16384
class Blob:
    def __init__(self, first_rom_bank=2):
        self.banks = []; self.first = first_rom_bank
    def new_bank(self):
        self.banks.append(bytearray()); return len(self.banks) - 1
    def cur(self):
        if not self.banks: self.new_bank()
        return len(self.banks) - 1
    def put(self, data, new_bank=False):
        """append; never straddles a bank. returns (rom_bank, offset)"""
        assert len(data) <= BANK
        b = self.cur()
        if new_bank and len(self.banks[b]): b = self.new_bank()
        if len(self.banks[b]) + len(data) > BANK: b = self.new_bank()
        off = len(self.banks[b]); self.banks[b] += data
        return self.first + b, off
    def put_stream(self, chunks):
        """chunks placed sequentially; when a chunk doesn't fit, a 0xFF marker sends the
        reader to the next bank. returns (rom_bank, offset) of the first chunk"""
        start = None
        for c in chunks:
            assert len(c) < BANK
            b = self.cur()
            if len(self.banks[b]) + len(c) + 1 > BANK:
                self.banks[b].append(0xFF); b = self.new_bank()
            if start is None: start = (self.first + b, len(self.banks[b]))
            self.banks[b] += c
        return start
    def at(self, rom_bank, off, data):
        b = rom_bank - self.first
        self.banks[b][off:off + len(data)] = data
    def bytes(self):
        out = bytearray()
        for b in self.banks: out += b + bytes(BANK - len(b))
        return bytes(out)
