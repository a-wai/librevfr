all:
	make -C resources
	make -C src

clean:
	make -C resources clean
	make -C src clean
