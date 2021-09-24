#!/usr/bin/env -S bash ../.port_include.sh
port=libcss
version=0.9.1
files="http://git.netsurf-browser.org/libcss.git/snapshot/libcss-release/${version}.tar.bz2 libcss-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libcss-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
