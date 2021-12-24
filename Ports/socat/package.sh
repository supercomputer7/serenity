#!/usr/bin/env -S bash ../.port_include.sh
port=socat
useconfigure="true"
version="1.7.4.2"
makeopts=("-j1" "LDFLAGS=")
files="http://www.dest-unreach.org/socat/download/socat-${version}.tar.gz socat-${version}.tar.gz"
workdir="socat-${version}"
configopts=("--disable-tun"
            "--disable-vsock"
            "--disable-sctp"
            "--disable-gopen"
            "--disable-unix"
            "--disable-ip6"
            "--disable-rawip"
            "--disable-genericsocket"
            "--disable-interface"
            "--disable-abstract-unixsocket"
            "--disable-proxy"
            "--disable-readline"
            "--disable-openssl"
            "--disable-retry"
            "--disable-libwrap"
            "--target=SerenityOS"
)

build() {
    run make "${makeopts[@]}"
}
