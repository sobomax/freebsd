#!/bin/sh

#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2026 Maksym Sobolyev <sobomax@FreeBSD.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

# rename(2) of a directory to a new parent when ".." cannot be rewritten.
#
# ufs_rename() adds a link to the new parent for the moved directory's
# "..", and with SU+J a journal record (jaddref) for it, and then rewrites
# "..".  When that rewrite failed, for example on a read error, ".." still
# named the old parent, but the new parent kept the extra link and the
# journal record was never written.  With SU+J the record kept the new
# parent's link count rolled back in every write of its inode, so the inode
# block was redirtied forever: unmount failed with EAGAIN once fsync gave up
# on it, and a forced unmount after I/O errors retried forever.
# Without the journal, fsck found the new parent's link count too high.
#
# The read of the moved directory's block is made to fail with gnop(8),
# after everything else the rename needs is cached.  fsck_ffs then has to
# report the stale "..", which shows that the rewrite failed, and nothing
# else.
#
# The move is tried to a free name and onto an existing empty directory.
# In the second case no link is added to the new parent, as the new ".."
# takes over the link of the replaced directory's "..".  When the rewrite
# failed, that link stayed on the new parent.

[ `id -u` -ne 0 ] && echo "Must be root!" && exit 1

. ../default.cfg

kldstat -q -m g_nop || gnop load || exit 1
log=/tmp/rename19.log
s=0

# mv(1) moves into an existing directory rather than replacing it.
cat > /tmp/rename19.c <<EOF
#include <err.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{

	if (argc != 3)
		errx(1, "usage: rename19 from to");
	if (rename(argv[1], argv[2]) == -1)
		err(1, "rename(%s, %s)", argv[1], argv[2]);
	return (0);
}
EOF
mycc -o /tmp/rename19 -Wall -Wextra -O2 /tmp/rename19.c || exit 1
rm /tmp/rename19.c

for flags in -U -j; do
for target in free existing; do
	t="$flags, $target target"
	set -e
	mount | grep -q "on $mntpoint " && umount -f $mntpoint
	mdconfig -l | grep -q "md$mdstart" && mdconfig -d -u $mdstart
	mdconfig -a -t swap -s 1g -u $mdstart
	newfs $flags /dev/md$mdstart > /dev/null
	gnop create /dev/md$mdstart
	mount /dev/md$mdstart.nop $mntpoint
	mkdir -p $mntpoint/a/d $mntpoint/b
	[ $target = existing ] && mkdir $mntpoint/b/d
	# Drop the cached blocks, then cache all but the block of a/d.
	umount $mntpoint
	mount /dev/md$mdstart.nop $mntpoint
	set +e
	ls -la $mntpoint $mntpoint/a $mntpoint/b > /dev/null
	[ $target = existing ] && ls -la $mntpoint/b/d > /dev/null
	stat $mntpoint/a/d > /dev/null

	gnop configure -r 100 md$mdstart.nop
	/tmp/rename19 $mntpoint/a/d $mntpoint/b/d
	gnop configure -r 0 md$mdstart.nop

	umount $mntpoint &
	upid=$!
	n=0
	while kill -0 $upid 2>/dev/null; do
		sleep 1
		n=$((n + 1))
		if [ $n -eq 120 ]; then
			echo "FAIL ($t): umount $mntpoint is stuck"
			procstat -kk $upid
			# The mount point stays busy, leave everything as is.
			exit 1
		fi
	done
	if ! wait $upid; then
		# The suspension gave up on the inode block of b.
		echo "FAIL ($t): umount $mntpoint failed"
		exit 1
	fi

	fsck_ffs -fn /dev/md$mdstart.nop > $log 2>&1
	if ! grep -q "'\.\.'" $log; then
		echo "SKIP ($t): the rewrite of \"..\" did not fail"
		cat $log
	elif grep -q "LINK COUNT" $log; then
		echo "FAIL ($t): link count left wrong"
		cat $log
		s=1
	fi
	gnop destroy md$mdstart.nop
	mdconfig -d -u $mdstart
done
done
rm -f $log /tmp/rename19
exit $s
