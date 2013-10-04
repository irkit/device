
OUT = ir-state.png http-state.png ir-learn.png ir-send.png packer.png

all: $(OUT)

.SUFFIXES: .png .gv .msc

.gv.png:
	dot -Tpng $< -o $@

.msc.png:
	mscgen -T png -i $<
