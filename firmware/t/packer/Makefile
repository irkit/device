
test:
	g++ -Wall -g -o .build/test.out IrPacker.c test.cpp
	./.build/test.out

all:
	# g++ -Wall -g -o pack.bin IrPacker.c pack.cpp
	# g++ -Wall -g -o unpack.bin IrPacker.c unpack.cpp
	g++ -Wall -g -o unpack_sequence.bin IrPacker.c unpack_sequence.cpp

packer:
	g++ -Wall -g -o pack_sequence.bin IrPacker.c pack_sequence.cpp base64encoder.c

