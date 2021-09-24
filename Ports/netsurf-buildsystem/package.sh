#!/usr/bin/env -S bash ../.port_include.sh
port=netsurf-buildsystem
version=1.9
files="http://git.netsurf-browser.org/buildsystem.git/snapshot/buildsystem-release/${version}.tar.bz2 netsurf-buildsystem.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=buildsystem-release/${version}

build() {
    run echo ls
    run make CC="${CC}"
}

install() {
    run make CC="${CC}" $installopts install
}
