#!/usr/bin/env bash

set -e
set -x

PREFIX="$(pwd)"
TARGET=ia16-elf

if [ -z "$MAKEFLAGS" ]; then
	MAKEFLAGS="$1"
fi
export MAKEFLAGS

export PATH="$PREFIX/bin:$PATH"

if [ -x "$(command -v gmake)" ]; then
    mkdir -p "$PREFIX/bin"
    cat <<EOF >"$PREFIX/bin/make"
#!/usr/bin/env sh
gmake "\$@"
EOF
    chmod +x "$PREFIX/bin/make"
fi

mkdir -p build
cd build

git clone https://github.com/tkchia/binutils-ia16.git --depth=1 || true
pushd binutils-ia16 && git pull && popd
git clone https://github.com/tkchia/gcc-ia16.git --depth=1 || true
pushd gcc-ia16 && git pull && popd

rm -rf build-gcc build-binutils

mkdir build-binutils
cd build-binutils
../binutils-ia16/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make all-binutils all-ld all-gas
make install-binutils install-ld install-gas
cd ..

cd gcc-ia16
contrib/download_prerequisites
cd ..
mkdir build-gcc
cd build-gcc
../gcc-ia16/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers --disable-multilib
make all-gcc
make install-gcc
make all-target-libgcc
make install-target-libgcc
