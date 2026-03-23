# Maintainer: Your Name <your-email@example.com>

pkgname=paccy
pkgver=1.0.0
pkgrel=1
pkgdesc="GUI installer for Arch Linux pacman packages, similar to Eddy"
arch=('x86_64')
url="https://github.com/cloudwithax/paccy"
license=('GPL3')
depends=('qt6-base' 'polkit')
makedepends=('cmake' 'qt6-tools')
source=("https://github.com/cloudwithax/$pkgname/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('09fce55e13918a97cd3065a2a47e70f5b2e39c9e6378eabe25c0d6195bfd2dcc')

build() {
    cmake -B build -S "$pkgname-$pkgver" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
