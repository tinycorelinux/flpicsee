#makefile for fltk program
CXXFLAGS="-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti"
PROG=flpicsee
PROG_CAPS=FL-PicSee
DESC="Picture Viewer"
TCEDIR=`cat /opt/.tce_dir`
BASEDIR=`pwd`

# Configurations: 
# run 'make' to build with standard options 
# run 'make basic' to build with (all?) options disabled

#Standard Version: has right-click menu and multi-image paging. About 23.0K
#Uses these #defines: (none) 
all: 
	gcc ${CXXFLAGS} `fltk-config --cxxflags` -c  ${PROG}.cpp
	gcc `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
	sudo cp ${PROG} /usr/local/bin

#Basic Version: no right-click menu, etc.  About 14.6K in TC 3.0
#Uses these #defines: -DNO_PAGING -DNO_MENU
basic: 
	gcc ${CXXFLAGS} -DNO_MENU -DNO_PAGING `fltk-config --cxxflags` -c  ${PROG}.cpp
	gcc `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
	sudo cp ${PROG} /usr/local/bin
	
#No Paging Version: about 17.2K in TC 3.0
#Uses these #defines: -DNO_PAGING
nopaging: 
	gcc ${CXXFLAGS} -DNO_PAGING `fltk-config --cxxflags` -c  ${PROG}.cpp
	gcc `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
	sudo cp ${PROG} /usr/local/bin
	
#No Menu Version: about 18.4K in TC 3.0
#Uses these #defines: -DNO_MENU
nomenu: 
	gcc ${CXXFLAGS} -DNO_MENU `fltk-config --cxxflags` -c  ${PROG}.cpp
	gcc `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
	sudo cp ${PROG} /usr/local/bin
	
package:
	./build-tcz.sh ${PROG} ${PROG_CAPS} ${BASEDIR} ${DESC}

deploy: package
	echo "TCE DIR is ${TCEDIR}"
	cp ${BASEDIR}/${PROG}.tcz ${TCEDIR}/optional

clean:
	rm *.o ${PROG}
	
tarball:
	tar -czf flpicsee_src.tar.gz flpicsee.cpp flpicsee.png flpicsee.tcz.info build-tcz.sh package.sh Makefile
