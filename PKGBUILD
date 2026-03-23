# Maintainer: Your Name <your-email@example.com>

pkgname=paccy
pkgver=1.0.0
pkgrel=1
pkgdesc="GUI installer for Arch Linux pacman packages, similar to Eddy"
arch=('x86_64')
url="https://github.com/YOUR_USERNAME/paccy"
license=('GPL3')
depends=('qt6-base' 'polkit')
makedepends=('cmake' 'qt6-tools')
source=("https://github.com/YOUR_USERNAME/$pkgname/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cmake -B build -S "$pkgname-$pkgver" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
