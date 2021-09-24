#!/usr/bin/env -S bash ../.port_include.sh
port=libnsbmp
version=0.1.6
files="http://git.netsurf-browser.org/libnsbmp.git/snapshot/libnsbmp-release/${version}.tar.bz2 libnsbmp-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libnsbmp-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
