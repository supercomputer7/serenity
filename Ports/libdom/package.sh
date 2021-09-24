#!/usr/bin/env -S bash ../.port_include.sh
port=libdom
version=0.4.1
files="http://git.netsurf-browser.org/libdom.git/snapshot/libdom-release/${version}.tar.bz2 libdom-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libdom-release/${version}
depends="libexpat"

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
