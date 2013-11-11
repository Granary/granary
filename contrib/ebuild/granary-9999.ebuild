# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

inherit eutils git-2 python

DESCRIPTION="A kernel space dynamic binary translatino framework"
HOMEPAGE="https://github.com/Granary/granary"
#SRC_URI=""
EGIT_PROJECT="granary"
EGIT_REPO_URI="https://github.com/Granary/granary"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~amd64"
IUSE="kernel clang null even_odd single_even_odd null_plus instr_dist track_entry_exit cfg watchpoint_null watchpoint_user watchpoint_stats everything_watched everything_watch_aug bounds_checker leak_detector shadow_memory rcudbg watchpoint_leak trace"

#Python specifics
PYTHON_DEPEND="2:2.7"

DEPEND=""
RDEPEND="${DEPEND}"

REQUIRED_USE="^^ ( null even_odd single_even_odd null_plus instr_dist track_entry_exit cfg watchpoint_null watchpoint_user watchpoint_stats everything_watched everything_watch_aug bounds_checker leak_detector shadow_memory rcudbg watchpoint_leak )"

src_prepare() {
	COMMIT=$(git rev-parse HEAD)
	if [[ $COMMIT == "6713ad981665830009b043fb5cbf25c0c10ca671" ]]; then
		epatch "${FILESDIR}/${COMMIT}.patch"
		epatch "${FILESDIR}/${COMMIT}-globals.patch"
	fi
}

src_compile() {
	if use kernel; then
		KERNEL=1
		GR_DLL=0
	else
		KERNEL=0
		GR_DLL=1
	fi
	KERNEL_DIR="/usr/src/linux"

	if use clang; then
		GR_CC="clang"
		GR_CXX="clang++"
	else
		GR_CC="gcc"
		GR_CXX="g++"
	fi

	if use trace; then
		CFLAGS="-DCONFIG_TRACE_EXECUTION"
	else
		CFLAGS="-fPIC"
	fi

	GR_PYTHON=$(PYTHON)
	GR_CLIENT=$(usev null || usev even_odd || usev single_even_odd || usev null_plus || usev instr_dist || usev track_entry_exit || usev cfg || usev watchpoint_null || usev watchpoint_user || usev watchpoint_stats || usev everything_watched || usev everything_watch_aug || usev bounds_checker || usev leak_detector || usev shadow_memory || usev rcudbg || usev watchpoint_leak)
	
	emake env KERNEL=$KERNEL GR_DLL=$GR_DLL KERNEL_DIR=$KERNEL_DIR GR_CC=$GR_CC GR_CXX=$GR_CXX GR_PYTHON=$GR_PYTHON GR_CLIENT=$GR_CLIENT

	if use kernel; then
		$(PYTHON) scripts/slowload.py --symbols
	fi
	emake detach KERNEL=$KERNEL GR_DLL=$GR_DLL KERNEL_DIR=$KERNEL_DIR GR_CC=$GR_CC GR_CXX=$GR_CXX GR_PYTHON=$GR_PYTHON GR_CLIENT=$GR_CLIENT
	emake wrappers KERNEL=$KERNEL GR_DLL=$GR_DLL KERNEL_DIR=$KERNEL_DIR GR_CC=$GR_CC GR_CXX=$GR_CXX GR_PYTHON=$GR_PYTHON GR_CLIENT=$GR_CLIENT
	emake clean KERNEL=$KERNEL GR_DLL=$GR_DLL KERNEL_DIR=$KERNEL_DIR GR_CC=$GR_CC GR_CXX=$GR_CXX GR_PYTHON=$GR_PYTHON GR_CLIENT=$GR_CLIENT
	emake all KERNEL=$KERNEL GR_DLL=$GR_DLL KERNEL_DIR=$KERNEL_DIR GR_CC=$GR_CC GR_CXX=$GR_CXX GR_PYTHON=$GR_PYTHON GR_CLIENT=$GR_CLIENT GR_EXTRA_CC_FLAGS="$CFLAGS" GR_EXTRA_CXX_FLAGS="$CFLAGS"
}

src_install() {
	rm -rf /tmp/granary
	mkdir -p /tmp/granary
	cp ${WORKDIR}/${P}/bin/libgranary.so /tmp/granary/
	cp ${WORKDIR}/${P}/.gdbinit /tmp/granary/
	elog "The granary shared library is located at /tmp/granary/libgranary.so"
	elog "This was installed assuming your kernel was prepared according to
	https://github.com/Granary/granary/blob/master/docs/building-the-kernel.md"
}
