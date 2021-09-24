#!/usr/bin/env -S bash ../.port_include.sh
port=netsurf
version=3.10
configopts="--enable-shared --disable-nls"
files="http://git.netsurf-browser.org/netsurf.git/snapshot/netsurf-release/${version}.tar.bz2 netsurf-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=netsurf-release/${version}
depends="zlib freetype openssl curl libiconv libjpeg libpng netsurf-buildsystem libwapcaplet libparserutils libhubbub libcss libdom libnsbmp libnsgif"

build() {
    run echo ls
    run make CC="${CC}" $installopts TARGET=framebuffer
}

install() {
    run make  CC="${CC}" $installopts install TARGET=framebuffer
}
