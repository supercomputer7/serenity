#!/usr/bin/env -S bash ../.port_include.sh
port=libnsgif
version=0.2.1
files="http://git.netsurf-browser.org/libnsgif.git/snapshot/libnsgif-release/${version}.tar.bz2 libnsgif-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libnsgif-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
