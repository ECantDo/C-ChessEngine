EXE = chessEngine

.PHONY: all

all:
	mkdir -p _build
	cd _build && cmake .. -DCMAKE_BUILD_TYPE=Release
	cd _build && $(MAKE) -j$$(nproc)
	cp _build/chessEngine $(EXE)