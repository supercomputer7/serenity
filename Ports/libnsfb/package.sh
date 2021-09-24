#!/usr/bin/env -S bash ../.port_include.sh
port=libnsfb
version=0.2.2
files="http://git.netsurf-browser.org/libnsfb.git/snapshot/libnsfb-release/${version}.tar.bz2 libnsfb-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libnsfb-release/${version}
depends="libexpat"

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
