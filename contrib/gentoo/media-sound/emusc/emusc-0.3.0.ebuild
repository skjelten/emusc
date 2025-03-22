# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit cmake-multilib xdg

DESCRIPTION="EmuSC is a software synthesizer that aims to emulate the Roland Sound Canvas SC-55 lineup."
HOMEPAGE="https://github.com/skjelten/emusc"

if [[ ${PV} == 9999 ]]; then
	EGIT_REPO_URI="https://github.com/skjelten/${PN}.git"
	inherit git-r3
else
	SRC_URI="https://github.com/skjelten/${PN}/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="~amd64 ~x86"
fi

LICENSE="GPL-3+"
SLOT="0"
IUSE="+alsa +pulseaudio +wav jack +qtcharts +qtmultimedia"

DEPEND="
	dev-qt/qtbase:6[gui,widgets]
	dev-qt/qtmultimedia
	dev-qt/qtcharts
	media-libs/libemusc
	alsa? ( media-libs/alsa-lib )
	pulseaudio? ( media-libs/libpulse )
	jack? ( virtual/jack )
"
RDEPEND="${DEPEND}"

S="${WORKDIR}/${P}/${PN}"

DOCS="${S}/README.md ${S}/AUTHORS ${S}/ChangeLog ${S}/NEWS"

multilib_src_configure() {
        local mycmakeargs=(
		-DQT_AUDIO="$(usex qtmultimedia)"
		-DUSE_QTCHARTS="$(usex qtcharts)"
		-DWAV_AUDIO="$(usex wav)"
		-DALSA_AUDIO="$(usex alsa)"
		-DALSA_MIDI="$(usex alsa)"
		-DPULSE_AUDIO="$(usex pulseaudio)"
		-DJACK_AUDIO="$(usex jack)"
		-DCMAKE_BUILD_TYPE=Release
        )
        cmake_src_configure
}

multilib_src_install() {
	cmake_src_install
	rm -f ${D}/usr/share/doc/${PN}/AUTHORS
	rm -f ${D}/usr/share/doc/${PN}/ChangeLog
	rm -f ${D}/usr/share/doc/${PN}/COPYING
	rm -f ${D}/usr/share/doc/${PN}/NEWS
	rm -f ${D}/usr/share/doc/${PN}/README.md
}

pkg_preinst() {
	xdg_pkg_preinst
}

pkg_postinst() {
	xdg_pkg_postinst
}

pkg_postrm() {
	xdg_pkg_postrm
}
