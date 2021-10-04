; Boot sector for floppies.
; Loads a file, assumes it is not fragmented and located in the first 32 root entries.
cpu 8086
org 7C00h
bits 16

    jmp short bootloader
    nop

bpb:
    .oem_name db 'LibreDOS'
    .bytes_per_sector dw 0x0200
    .sectors_per_cluster db 0x01
    .reserved_sectors dw 0x0001
    .fat_count db 0x02
    .root_entries dw 0x00E0
    .sector_count dw 0x0B40
    .media_descriptor db 0xF0
    .sectors_per_fat dw 0x0009
    .sectors_per_track dw 0x0012
    .head_count dw 0x0002
    .hidden_sectors dd 0x00000000
    .total_sectors dd 0x00000000
    .drive_number db 0x00
    .reserved db 0x00
    .boot_signature db 0x29
    .serial_number dd 0x00000000
    .volume_label db 'LibreDOS   '
    .file_system db 'FAT12   '

bootloader:
    cli
    cld
    jmp 0x0000:.jump
  .jump:
    push cs                             ; Set ds=cs
    pop ds
    push cs                             ; Set up stack just below boot sector
    pop ss
    mov sp, 0x7C00
    mov ax, 0x0050                      ; Set es to the place we want to load our stuff
    mov es, ax

    mov si, message                     ; Tell the user we somehow got this far
    call print_string
    push [.drive_number]                ; Try the drive number the BIOS gave us first
    call loadFile
    pop [.drive_number]
    call loadFile
    xor ah, ah                          ; Waive the white flag
    int 16h
    int 18h
    jmp $

; Increase int 13h arguments
inc_chs:
    push cx                             ; Check for track overflow
    and cl, 0x3F
    cmp cl, [bpb.sectors_per_track]
    pop cx
    jae short .next_track
    inc cl                              ; Increase sector number
    ret
  .next_track:
    and cl, 0xC0                        ; Reset track number
    inc cl
    inc dh                              ; Increase head number
    cmp dh, [bpb.head_count]
    jae short .next_cylinder
    ret
    inc ch                              ; Increase cylinder number
    jc short .return
    add cl, 0x40
  .return:
    ret

; Turn the LSN into ax into int 13h CSH coordinates, and load dl with the tentative boot drive.
lsn2chs:
    push bx
    add ax, [bpb.hidden_sectors]        ; Convert LSN to LBA
    xor dx, dx                          ; track_number = lsn/sectors_per_tracke and sector = lsn%sectors_per_track+11
    mov bx, [bpb.sectors_per_track]
    div bx
    mov cl, dl                          ; Store sector number
    inc cl
    xor dx, dx                          ; cylinder = track_number/head_count and head = track_number%head_count
    xor bh, bh
    mov bl, [bpb.head_count]
    div bx
    mov dh, dl                          ; Store head number
    mov dl, [bpb.drive_number]          ; Store drive number
    mov ch, al                          ; Store cylinder number
    test ah, 0x01
    jz short .nope1
    or cl, 0x40
  .nope1:
    test ah, 0x02
    jz short .nope2
    or cl, 0x80
  .nope2:
    mov ax, 0x0201
    pop bx
    ret

print_string:
    lodsb
    test al, al
    jz short .break
    mov ah, 0x09
    xor bx, bx
    int 10h
    jmp print_string
  .break:
    ret

filename db 'KERNEL  BIN'
message db 'Loading LibreDOS ...', 0x0D, 0x0A, 0x00
errorMsg db 'Non system disk or disk error! Press any key ...', 0x0D, 0x0A, 0x00

times 510-($-$$) db 0x00
db 0x55, 0xAA
