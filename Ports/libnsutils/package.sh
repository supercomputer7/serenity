#!/usr/bin/env -S bash ../.port_include.sh
port=libnsutils
version=0.1.0
files="http://git.netsurf-browser.org/libnsutils.git/snapshot/libnsutils-release/${version}.tar.bz2 libnsutils-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libnsutils-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
