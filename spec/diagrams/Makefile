
OUT = ir-state.png

all: $(OUT)

.SUFFIXES: .png .gv

.gv.png:
	dot -Tpng $< -o $@
