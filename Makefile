ISO_IMAGE = libredos.iso

.PHONY: all $(ISO_IMAGE) clean run

all: $(ISO_IMAGE)

$(ISO_IMAGE): limine
	$(MAKE) -C kernel
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/kernel.bin \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-install $(ISO_IMAGE)
	rm -rf iso_root

clean:
	$(MAKE) clean -C kernel
	rm -f $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-x86_64 -M q35 -m 2G -cdrom $(ISO_IMAGE)

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	make -C limine
