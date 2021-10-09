; Boot sector for floppies.
; Loads a file, assumes it is not fragmented and located in the first 32 root entries.
; The file is loaded at address 0x00500, and must thus be less than 30464 bytes (minus stack) in size.
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
    .volume_label db 'LIBREDOS   '
    .file_system db 'FAT12   '

bootloader:
    cli
    cld
    jmp 0x0000:.jump
  .jump:
    mov ax, cs                          ; Set ds=cs=ss
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00                      ; Set up stack just below boot sector
    mov ax, 0x0150                      ; Set es to the place we want to load our stuff
    mov es, ax

    mov si, message                     ; Tell the user we somehow got this far
    call print_string
    push word [bpb.drive_number]        ; Try the drive number the BIOS gave us first
    mov [bpb.drive_number], dl
    call loadFile
    pop word [bpb.drive_number]
    call loadFile
    mov si, errorMsg                    ; Waive the white flag
    call print_string
    xor ah, ah
    int 16h
    int 18h
    jmp $

loadFile:
    mov ax, [bpb.sectors_per_fat]       ; Calculate first root directory table sector
    xor bh, bh
    mov bl, [bpb.fat_count]
    mul bx
    add ax, [bpb.reserved_sectors]
    mov bp, ax                          ; Save LSN
    call lsn2chs                        ; Load root directory
    xor di, di
  .search:
    mov si, filename                    ; Search for file
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je short .found
  .next:
    add di, 0x0020
    cmp di, 0x0200                      ; Check for end of sector
    jb short .search
  .nosign:
    ret
  .found:
    mov ax, [es:di+0x1A]                ; Load cluster number
    test ax, ax                         ; Check if CN is not zero
    jz short .next
    dec ax                              ; Calculate LSN
    dec ax
    mov bl, [bpb.sectors_per_cluster]   ; bx (and thus bh) should still be zero
    mul bx
    add ax, bp
    mov bx, [bpb.root_entries]
    mov cl, 0x04
    shr bx, cl
    add ax, bx
    mov bp, [es:di+0x1C]                ; Get file size in bytes
    mov di, bp
    test bp, 0x01FF                     ; Calculate file size in sectors
    jnz short .odd
    dec bp
  .odd:
    mov cl, 0x09
    shr bp, cl
    inc bp
    call lsn2chs                        ; Load first sector
  .loop:
    dec bp                              ; Check if all sectors loaded
    jz short .done
    call inc_chs                        ; Load next sector
    jmp .loop
  .done:
    mov ax, 0x55AA                      ; Search for entry point signature
    mov cx, di
    shr cx,1
    xor di, di
  .continue:
    repne scasw
    jne .nosign
    cmp word [es:di], 0x55AA           ; Check for second signature word
    jne .continue
    push es                            ; Transfer control
    lea ax, [di+0x0002]
    push ax
    mov bp, sp
    jmp far [bp]

; Increase int 13h arguments
inc_chs:
    add bx, 0x0200                      ; Increase offset
    push cx                             ; Check for track overflow
    and cl, 0x3F
    cmp cl, [bpb.sectors_per_track]
    pop cx
    jae short .next_track
    inc cl                              ; Increase sector number
    jmp int13
  .next_track:
    and cl, 0xC0                        ; Reset track number
    inc cl
    inc dh                              ; Increase head number
    cmp dh, [bpb.head_count]
    jae short .next_cylinder
    jmp int13
  .next_cylinder:
    xor dh, dh                          ; Reset head number
    inc ch                              ; Increase cylinder number
    jnz short .return
    add cl, 0x40
  .return:
    jmp int13

; Turn the LSN in ax into int 13h CSH coordinates, and load dl with the tentative boot drive.
lsn2chs:
    add ax, [bpb.hidden_sectors]        ; Convert LSN to LBA
    xor dx, dx                          ; track_number = lsn/sectors_per_track and sector = lsn%sectors_per_track+11
    div word [bpb.sectors_per_track]
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
    xor bx, bx
int13:
    mov si, 0x0003                      ; Reset retry counter
  .retry:
    mov ax, 0x0201                      ; Execute int 13h
    int 13h
    jc short .error
    ret
  .error:
    dec si                              ; Check if we should retry it
    jz short .abort
    xor ah, ah                          ; Reset controller before retry
    int 13h
    jmp .retry
  .abort:
    pop ax                              ; Return to main function
    ret

print_string:
    lodsb
    test al, al
    jz short .break
    mov ah, 0x0E
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
