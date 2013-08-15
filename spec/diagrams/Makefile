
OUT = ir-state.png packer.png

all: $(OUT)

.SUFFIXES: .png .gv

.gv.png:
	dot -Tpng $< -o $@
