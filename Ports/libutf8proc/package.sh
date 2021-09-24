#!/usr/bin/env -S bash ../.port_include.sh
port=libutf8proc
version=2.4.0-1
files="http://git.netsurf-browser.org/libutf8proc.git/snapshot/libutf8proc-release/${version}.tar.bz2 libutf8proc-${version}.tar.bz2"
installopts="PREFIX=${SERENITY_INSTALL_ROOT}/usr/local"
workdir=libutf8proc-release/${version}

build() {
    run echo ls
    run make CC="${CC}" $installopts
}

install() {
    run make  CC="${CC}" $installopts install
}

