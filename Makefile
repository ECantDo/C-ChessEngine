EXE  = chessEngine
ARCH = native

ifeq ($(OS), Windows_NT)
	BINARY = _build/chessEngine.exe
	MKDIR  = if not exist _build mkdir _build
	CP     = copy /Y
	JOBS   = 4
else
	BINARY = _build/chessEngine
	MKDIR  = mkdir -p _build
	CP     = cp
	JOBS   = $(shell nproc)
endif

.PHONY: all clean

all:
	$(MKDIR)
	cd _build && cmake .. -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$(ARCH)
	cd _build && $(MAKE) -j$(JOBS)
	$(CP) $(BINARY) $(EXE)

clean:
	rm -rf _build $(EXE) $(EXE).exe