all: test


test: build
	src/callback/callback
	src/callback/callback2
	

build:
	make -C src/callback all
