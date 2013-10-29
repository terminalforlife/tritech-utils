#!/bin/sh


test "$UID" != "0" && echo "You must be root to auto-build a 32-bit package." && exit 1
export CHROOT=/x32
export WD=$(pwd)

if [ "$DO_32BIT_BUILD" = "1" ]
	then
	unset DO_32BIT_BUILD
	cd $1
	echo $1
	make clean
	make package || echo "Build failed!"
	make clean
	echo "32-bit build finished."
	exit
fi

uname -m | grep -q 'i.86' && exit
test ! -d $CHROOT && echo "$CHROOT chroot not present, not building i386 package." && exit 1
test ! -x $CHROOT/bin/sh && echo "$CHROOT does not seem to be a chroot; aborting." && exit 1

mount --bind /dev $CHROOT/dev
mount --bind /usr/src $CHROOT/usr/src
mount --bind /home $CHROOT/home
mount -t proc proc $CHROOT/proc
mount -t sysfs sysfs $CHROOT/sys
mount -t tmpfs tmpfs $CHROOT/tmp
export DO_32BIT_BUILD=1
linux32 chroot $CHROOT $WD/$0 $WD
umount $CHROOT/proc $CHROOT/sys $CHROOT/tmp $CHROOT/dev $CHROOT/usr/src $CHROOT/home

