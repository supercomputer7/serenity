#!/usr/bin/env -S bash ../.port_include.sh
port=libhubbub
version=0.3.7
files="http://git.netsurf-browser.org/libhubbub.git/snapshot/libhubbub-release/${version}.tar.bz2 libhubbub-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libhubbub-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
