DEVKIT ?= $(HOME)/.devkitsms
CC := sdcc
CFLAGS := -mz80 -I$(DEVKIT)/include --peep-file $(DEVKIT)/include/peep-rules.txt -Isrc -Igen --max-allocs-per-node 20000 --opt-code-speed
LDFLAGS := -mz80 --no-std-crt0 --data-loc 0xC000
OBJS := build/main.rel build/game.rel build/fmv.rel build/vdp.rel build/sound.rel build/music_data.rel build/sfx_data.rel build/assets.rel

all: rom/t2k_sms.sms

build/%.rel: src/%.c src/*.h gen/assets.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@
build/assets.rel: gen/assets.c gen/assets.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/code.ihx: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(DEVKIT)/lib/crt0_sms.rel $(OBJS) $(DEVKIT)/lib/SMSlib.lib $(DEVKIT)/lib/PSGlib.lib

build/code.sms: build/code.ihx
	$(DEVKIT)/bin/ihx2sms $< $@

rom/t2k_sms.sms: build/code.sms gen/blob.bin tools/mkrom.py
	@mkdir -p rom
	python3 tools/mkrom.py build/code.sms gen/blob.bin $@

playtest: rom/t2k_sms.sms
	$(DEVKIT)/bin/smstest rom/t2k_sms.sms tests/playtest.txt

clean:
	rm -rf build rom/t2k_sms.sms
