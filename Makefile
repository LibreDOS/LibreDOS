ISO_IMAGE = libredos.iso
FLOPPY_IMAGE = libredos.img

export PATH := $(shell pwd)/toolchain/bin:$(PATH)

.PHONY: all $(ISO_IMAGE) $(FLOPPY_IMAGE) clean run runfdd

all: $(ISO_IMAGE)

$(ISO_IMAGE): limine
	$(MAKE) -C kernel
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/kernel.bin \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-deploy $(ISO_IMAGE)
	rm -rf iso_root

$(FLOPPY_IMAGE):
	$(MAKE) -C kernel
	$(MAKE) -C bootsect
	rm -f $(FLOPPY_IMAGE)
	mkdosfs -C $(FLOPPY_IMAGE) -n LIBREDOS 1440
	dd if=bootsect/bootsect.bin of=$(FLOPPY_IMAGE) bs=1 count=11 conv=notrunc
	dd if=bootsect/bootsect.bin of=$(FLOPPY_IMAGE) bs=1 count=450 seek=62 skip=62 conv=notrunc
	mcopy -ovi $@ kernel/kernel.bin ::/
	mattrib -i $@ -a +r +s kernel.bin

clean:
	$(MAKE) clean -C kernel
	$(MAKE) clean -C bootsect
	rm -f $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-x86_64 -M q35 -m 2G -cdrom $(ISO_IMAGE)

runfdd: $(FLOPPY_IMAGE)
	qemu-system-i386 -fda $(FLOPPY_IMAGE)

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1
	$(MAKE) -C limine
