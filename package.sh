#!/bin/bash
#required extensions: compiletc, bash, advcomp
make
mkdir -p picsee/usr/local/bin
mkdir -p picsee/usr/local/tce.menu
cp flpicsee picsee/usr/local/bin/
echo "<JWM>" > picsee/usr/local/tce.menu/flpicsee
echo "<Program label=\"flPicSee\">/usr/local/bin/flpicsee</Program>" >> picsee/usr/local/tce.menu/flpicsee
echo "</JWM>" >> picsee/usr/local/tce.menu/flpicsee
cd picsee
find usr -not -type d > /tmp/list
tar -T /tmp/list -czvf ./flpicsee.tce
#cd /home/tc
advdef -z4 flpicsee.tce
#create info file manually
md5sum flpicsee.tce > flpicsee.tce.md5.txt
tar -ztf flpicsee.tce > flpicsee.tce.list
