all: src
.PHONY: all src clean

src:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
