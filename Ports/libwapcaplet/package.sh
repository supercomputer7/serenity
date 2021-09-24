#!/usr/bin/env -S bash ../.port_include.sh
port=libwapcaplet
version=0.4.3
files="http://git.netsurf-browser.org/libwapcaplet.git/snapshot/libwapcaplet-release/${version}.tar.bz2 libwapcaplet-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libwapcaplet-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
