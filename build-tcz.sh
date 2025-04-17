#!/bin/sh
# Build .tcz extension package for version 3.0 of Tinycore
echo "PROG is ${PROG}"
echo "PROG is `echo ${PROG}`"

PROG=$1
PROG_CAPS=$2
BASEDIR=$3
DESC=$4

mkdir /tmp/${PROG}
cd /tmp/${PROG}

# Executable
mkdir -p usr/local/bin
cp ${BASEDIR}/${PROG} usr/local/bin/
chmod 755 usr/local/bin/${PROG}

# Icon
mkdir -p usr/local/share/pixmaps
cp ${BASEDIR}/${PROG}.png usr/local/share/pixmaps

# .desktop file for icons on desctop, menu items
DOTDESKTOP=usr/local/share/applications/${PROG}.desktop
mkdir usr/local/share/applications
echo "[Desktop Entry]" > ${DOTDESKTOP}
echo "Encoding=UTF-8" >> ${DOTDESKTOP}
echo "Name=${PROG}" >> ${DOTDESKTOP}
echo "Comment=${DESC}" >> ${DOTDESKTOP}
echo "GenericName=${PROG_CAPS}" >> ${DOTDESKTOP}
echo "Exec=${PROG}" >> ${DOTDESKTOP}
echo "Icon=${PROG}" >> ${DOTDESKTOP}
echo "Terminal=false" >> ${DOTDESKTOP}
echo "StartupNotify=true" >> ${DOTDESKTOP}
echo "Type=Application" >> ${DOTDESKTOP}
echo "Categories=Utility;" >> ${DOTDESKTOP}
echo "X-FullPathIcon=/usr/local/share/pixmaps/${PROG}.png" >> ${DOTDESKTOP}
echo "******************************"
cat ${DOTDESKTOP}
echo "******************************"

cd usr/local
cd /tmp/${PROG}
mksquashfs . ../${PROG}.tcz

#cd $TMPDIR
find usr -not -type d > ../${PROG}.tcz.list

# Create md5 file
cd /tmp
md5sum ${PROG}.tcz > ${PROG}.tcz.md5.txt

cp /tmp/${PROG}.tcz* ${BASEDIR}
rm -rf /tmp/${PROG}
rm -rf /tmp/${PROG}.tcz*

echo "Build of .tcz for TC 3.0 is complete."
