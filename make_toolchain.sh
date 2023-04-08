#! /bin/sh

set -ex

TARGET=ia16-elf

srcdir="$(dirname "$0")"
test -z "$srcdir" && srcdir=.
srcdir="$(cd "${srcdir}" && pwd -P)"

cd "$srcdir"

if [ -z "$TARGET" ]; then
    set +x
    echo "TARGET not specified"
    exit 1
fi

if command -v gmake; then
    export MAKE=gmake
else
    export MAKE=make
fi

if [ -z "$CFLAGS" ]; then
    export CFLAGS="-O2 -pipe"
fi

unset CC
unset CXX

mkdir -p toolchain && cd toolchain
PREFIX="$(pwd -P)"

export MAKEFLAGS="-j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || psrinfo -tc 2>/dev/null || echo 1)"

export PATH="$PREFIX/bin:$PATH"

rm -rf build
mkdir build
cd build

[ -d binutils-ia16 ] || git clone https://github.com/tkchia/binutils-ia16.git --depth=1
[ -d gcc-ia16 ] || git clone https://github.com/tkchia/gcc-ia16.git --depth=1

rm -rf build-gcc build-binutils

mkdir build-binutils
cd build-binutils
../binutils-ia16/configure CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
$MAKE all-binutils all-ld all-gas
$MAKE install-binutils install-ld install-gas
cd ..

cd gcc-ia16
sed 's/ftp:/https:/g' <contrib/download_prerequisites >contrib/download_prerequisites.sed
mv contrib/download_prerequisites.sed contrib/download_prerequisites
chmod +x contrib/download_prerequisites
contrib/download_prerequisites
cd ..
mkdir build-gcc
cd build-gcc
../gcc-ia16/configure CFLAGS="$CFLAGS" CXXFLAGS="$CFLAGS" --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers --disable-multilib
$MAKE all-gcc
$MAKE all-target-libgcc
$MAKE install-gcc
$MAKE install-target-libgcc
