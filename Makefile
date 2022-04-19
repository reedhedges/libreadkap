

# Release: (Note disables assert()s.)
CXXFLAGS:=-std=c++17 -DNDEBUG -O3 -Wall -Wextra -Wpedantic -Wsign-conversion -Wconversion

# Debug:
#CXXFLAGS:=-std=c++17 -DDEBUG -g -Og -Wall -Wextra -Wpedantic -Wsign-conversion -Wconversion

all: libreadkap.a

libreadkap.a: libreadkap.o
	$(AR) -crs $@ $^

libreadkap.o: libreadkap.cpp libreadkap.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

example: libreadkap.a
	$(CXX) -o $@ $^ libreadkap.a -lfreeimage -lm

install: libreadkap.a
	$(INSTALL) -m 644  libreadkap.a /usr/local/lib

clean:
	rm libreadkap.a libreadkap.o

# (Re)-generate compile_commands.json which is used by tools like Intellisense (VS Code)
# clang-tidy, other linters/analyzers.  Regenerate this if VS code won't get rid of some 
# compile errors in the Problems panel.
# "bear" tool is required. Install with apt install bear.
compile_commands.json: Makefile clean
	bear -- $(MAKE)

#test: libreadkap
#	./libreadkap -w examples/image.png "52°20'" "3°40'" "1871;146" "51°40'" "1°10'" "50;934" -r "500;96-1918;96-1918;968-142;968-142;541" natl.kap
#	if [ -f natl.kap ]; then \
#	echo "libreadkap seems working correctly."; \
#	else \
#	echo "libreadkap seems broken."; \
#	fi
	
#testrc: libreadkap
#	./libreadkap ./examples/osm/L16-19056-38040-8-8_16.png ./examples/osm/L16-19056-38040-8-8_16.png.kap out.kap -t testchart -c
#	./libreadkap ./examples/osm/L16-19056-38040-8-8_16.png ./examples/osm/L16-19056-38040-8-8_16.png.kap out.kap -t testchart

.PHONY: install clean all test

