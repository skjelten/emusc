# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit cmake-multilib desktop

DESCRIPTION="EmuSC is a software synthesizer that aims to emulate the Roland Sound Canvas SC-55 lineup."
HOMEPAGE="https://github.com/skjelten/emusc"

MY_PN="emusc"

if [[ ${PV} == 9999 ]]; then
	EGIT_REPO_URI="https://github.com/skjelten/${MY_PN}.git"
	inherit git-r3
else
	SRC_URI="https://github.com/skjelten/${MY_PN}/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="~amd64 ~x86"
fi

LICENSE="LGPL-3+"
SLOT="0"

S="${WORKDIR}/${MY_PN}-${PV}/${PN}"

DOCS="${S}/README.md"

multilib_src_install() {
        cp ${BUILD_DIR}/"${MY_PN}.pc" ${S}
	cmake_src_install
	rm -f ${D}/usr/share/doc/${PF}/COPYING
	rm -f ${D}/usr/share/doc/${PF}/COPYING.LESSER
}