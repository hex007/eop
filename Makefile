all:
	@echo "Compiling PNG decoder"
	@gcc -c -lpng -lbmc_host -I/opt/vc/include -I./ loadpng.c

	@echo "Compiling JPEG decoder"
	@gcc -c -ljpeg -lbmc_host -I/opt/vc/include -I./ loadjpg.c

	@echo "Compiling eop executable"
	@gcc -lpng -ljpeg -L/opt/vc/lib/ -lbcm_host -lm -I./ -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux \
		loadpng.o loadjpg.o eop.c -o eop

clean:
	rm -f loadpng.o loadjpg.o eop

install:
	@echo "Installing to /usr/bin/"
	@cp ./eop /usr/bin/
