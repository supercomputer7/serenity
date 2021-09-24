#!/usr/bin/env -S bash ../.port_include.sh
port=libparserutils
version=0.2.4
files="http://git.netsurf-browser.org/libparserutils.git/snapshot/libparserutils-release/${version}.tar.bz2 libparserutils-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libparserutils-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}
