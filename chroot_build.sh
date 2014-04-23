#!/bin/sh

test "$(id -u)" != "0" && echo "You must be root to auto-build chroot packages." && exit 1
export CHROOT_ARCH="$1"
export CHROOT=/chroots/$CHROOT_ARCH
export WD=$(pwd)

if [ "$DO_CHROOT_BUILD" = "1" ]
	then
	cd $1
	echo $1
	make clean
	make package || echo "Build failed!"
	echo "$CHROOT_ARCH chroot build finished."
	exit
fi

test ! -d $CHROOT && echo "$CHROOT not present, not building $CHROOT_ARCH package." && exit 1
test ! -x $CHROOT/bin/sh && echo "$CHROOT does not seem to be a chroot; aborting." && exit 1

echo "Performing package build for $CHROOT"
mount --bind /dev $CHROOT/dev
mount --bind /usr/src $CHROOT/usr/src
mount --bind /home $CHROOT/home
mount -t proc proc $CHROOT/proc
mount -t sysfs sysfs $CHROOT/sys
mount -t tmpfs tmpfs $CHROOT/tmp
export DO_CHROOT_BUILD=1
if echo "$CHROOT_ARCH" | grep -q "i386"
	then linux32 chroot $CHROOT $WD/$0 $WD
	else chroot $CHROOT $WD/$0 $WD
fi
umount $CHROOT/proc $CHROOT/sys $CHROOT/tmp $CHROOT/dev $CHROOT/usr/src $CHROOT/home

