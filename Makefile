EXECUTABLE = pdp

all: build/Makefile
	@$(MAKE) -C build

build/Makefile:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release -DEXECUTABLE=${EXECUTABLE} ..

clean:
	@rm -rf build
	@rm -f ${EXECUTABLE}

run:
	OMP_PROC_BIND=true @./${EXECUTABLE}

.PHONY: all clean