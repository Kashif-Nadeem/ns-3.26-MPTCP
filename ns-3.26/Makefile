# Makefile wrapper for waf

all:
	./waf

# free free to change this part to suit your requirements
configure:
	./waf configure   --mptcp-example
build:
	./waf build       mptcp-example.cc
install:
	./waf install

clean:
	./waf clean

distclean:
	./waf distclean
