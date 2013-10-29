
OUT = ir-state.png ir-learn.png ir-send.png packer.png wifi-morse-setup.png

all: $(OUT)

.SUFFIXES: .png .gv .msc

.gv.png:
	dot -Tpng $< -o $@

.msc.png:
	mscgen -T png -i $<
