#!/bin/bash

#BASE_URL="git://git.enlightenment.org"
BASE_URL="git+ssh://git@git.enlightenment.org"

set -e
#set -x
shopt -s expand_aliases
unset LANG
export CFLAGS="-O0 -march=native -ffast-math -g3 -W -Wall -Wextra -mno-sse4" # -Wshadow"
alias make='make -j `echo \`grep -c '^processor' /proc/cpuinfo\`*2 | bc`'

# install efl in /usr/local/efl
#export PREFIX="/usr/local/efl"
#export PKG_CONFIG_PATH+="/usr/local/efl/lib/pkgconfig"

REPOS="core/efl core/elementary core/enlightenment core/evas_generic_loaders core/emotion_generic_players"

# manual repo selection
# ex) $  ./efl_script.sh status "core/evas_generic_loaders core/emotion_generic_players"
if [ -n "$2" ] ; then
	REPOS=$2
fi

function show_menu()
{
	echo "== EFL Git Script by SeoZ (modified by spacegrapher) =="
	echo "  1. clone"
	echo "  2. pull"
	echo "  3. build"
	read -p "Select options: " n

	case "$n" in
		1)
			clone_repos
			clone_finish
			;;
		2)
			pull_repos
			;;
		3)
			show_build_option
			build_repos
			build_finish
			;;
		*)
			;;
	esac
}

function show_build_option()
{
	echo ""
	echo "== Choose Your GL Options =="
	echo "  1. Full OpenGL"
	echo "  2. OpenGL ES"
	read -p "Select options: " n

	case "$n" in
		1)
			UBUNTU=1
			;;
		2)
			UBUNTU=2
			;;
		*)
			UBUNTU=1
			;;
	esac
}

function clone_repos()
{
	for REPO_ORG in $REPOS ; do
		REPO=`echo $REPO_ORG|cut -d'/' -f2`
		echo " "
		echo "============ clone "$REPO" ============"
		if [ ! -d "$REPO" ]; then
			git clone ${BASE_URL}/${REPO_ORG}.git ${REPO}
		else
			echo "Skipping existing repo: ${REPO}"
		fi
	done
}

function pull_repos()
{
	for REPO_ORG in $REPOS; do
		REPO=`echo $REPO_ORG|cut -d'/' -f2`
		echo " "
		echo "============ pull "$REPO" ============"
		pushd $REPO

		git pull --rebase
		popd
	done
}

function build_autotools()
{
	if [ -f Makefile ]; then
		make clean distclean || true
	fi
	if [ -z $PREFIX ]; then
		./autogen.sh $@
	else
		./autogen.sh $@ --prefix=$PREFIX
	fi
	make
	sudo make install
	sudo ldconfig
}

function build_repos()
{
	for REPO_ORG in $REPOS; do
		REPO=`echo $REPO_ORG|cut -d'/' -f2`
		echo " "
		echo "============ build "$REPO" ============"
		pushd $REPO
			if [ $REPO_ORG = "core/efl" ]; then
				case "$UBUNTU" in
					1)
						build_autotools --disable-gstreamer1 --disable-gstreamer --with-opengl=full --enable-i-really-know-what-i-am-doing-and-that-this-will-probably-break-things-and-i-will-fix-them-myself-and-send-patches-aba
						;;
					2)
						build_autotools --disable-gstreamer1 --disable-gstreamer --with-opengl=es --enable-egl --enable-i-really-know-what-i-am-doing-and-that-this-will-probably-break-things-and-i-will-fix-them-myself-and-send-patches-aba
						;;
				esac

			else
				build_autotools
			fi
		popd
	done
}

function clone_finish()
{
	echo "=============================================================="
	echo "= Congratulations. You cloned    EFL successfully!           ="
	echo "= Please run the following command to build and install efl. ="
	echo "=     $ ./efl_script.sh                                      ="
	echo "=     $ 3. build   <-   Select option 3                      ="
	echo "=============================================================="
}

function build_finish()
{
	echo "=============================================================="
	echo "= Congratulations. You installed EFL successfully!           ="
	echo "= Please run elementary_test to check if it is working well. ="
	echo "=============================================================="
}

show_menu
