default: release

.PHONY: default release debug all clean cppcheck

include make-utils/flags.mk
include make-utils/cpp-utils.mk

CXX_FLAGS += -pedantic
CXX_FLAGS += -Iinclude -Ilib/rapidxml/include

$(eval $(call auto_folder_compile,src))
$(eval $(call auto_add_executable,merger))

release: release_merger
release_debug: release_debug_merger
debug: debug_merger

all: release release_debug debug

cppcheck:
	cppcheck --enable=all --std=c++11 -I include src

clean: base_clean

run: release_debug/bin/merger
	./release_debug/bin/merger --ignore=Catch --ignore=test --ignore=lib xml/coverage_std.xml xml/coverage_mkl.xml target.xml

include make-utils/cpp-utils-finalize.mk
