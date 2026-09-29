#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2026 Maksym Sobolyev <sobomax@sippysoft.com>
#

# Tests for fsck_ffs(8) reconnecting orphaned inodes into lost+found.
#
# The orphans are made by building a file system image with makefs(8) that
# holds a directory full of entries and clearing that directory's inode with
# clri(8).  None of this needs root: fsck_ffs(8) checks the image file
# directly.

IMG=ffs.img

# Build $IMG with directory "d" holding $1 empty files and $2 directories,
# each of those with one file in it, and clear the inode of "d".  Small
# blocks make lost+found span many blocks with few entries.
make_orphans()
{
	local ino

	rm -rf root ${IMG}
	mkdir -p root/d
	(cd root/d && jot "$1" | xargs touch)
	if [ "$2" -gt 0 ]; then
		(cd root/d && jot "$2" | sed 's/^/dir/' | xargs mkdir)
		(cd root/d && jot "$2" | sed 's|^\(.*\)$|dir\1/f|' | xargs touch)
	fi
	atf_check -o ignore makefs -t ffs -o version=2,bsize=4096,fsize=512 \
	    -s 64m ${IMG} root
	ino=$(printf 'ls\n' | fsdb -r ${IMG} 2>/dev/null |
	    awk '/directory, `d'"'"'$/ { print $6 }')
	atf_check test -n "${ino}"
	atf_check -o ignore -e ignore clri ${IMG} ${ino}
}

# Repair $IMG, check that $1 inodes were reconnected and that the file
# system is clean afterwards, and set LOOKUPS to the number of block
# lookups fsck_ffs(8) made, taken from its debug statistics.
repair()
{
	atf_check -o save:fsck.out -e ignore fsck_ffs -d -fy ${IMG}
	atf_check -o inline:"$1\n" grep -c 'RECONNECT? yes' fsck.out
	atf_check -o not-match:'UNREF' -e ignore fsck_ffs -fn ${IMG}
	LOOKUPS=$(sed -n 's/^cache with .* missed [0-9]* of \([0-9]*\) .*/\1/p' \
	    fsck.out)
	atf_check test -n "${LOOKUPS}"
}

atf_test_case reconnect_files
reconnect_files_head()
{
	atf_set "descr" "Reconnecting orphans must not rescan lost+found " \
	    "for every inode"
	atf_set "require.progs" "makefs fsdb clri fsck_ffs jot"
}
reconnect_files_body()
{
	local l1 l2

	make_orphans 1000 0
	repair 1000
	l1=${LOOKUPS}
	make_orphans 2000 0
	repair 2000
	l2=${LOOKUPS}
	# Rescanning lost+found for every orphan makes the number of lookups
	# grow with the square of the number of orphans: doubling them about
	# quadruples it.  Without rescans it about doubles.
	if [ $((l2 * 10)) -ge $((l1 * 30)) ]; then
		atf_fail "lookups went from ${l1} for 1000 orphans to" \
		    "${l2} for 2000"
	fi
}

atf_test_case reconnect_dirs
reconnect_dirs_head()
{
	atf_set "descr" "Orphaned files and directories are all reconnected"
	atf_set "require.progs" "makefs fsdb clri fsck_ffs jot"
}
reconnect_dirs_body()
{
	make_orphans 2000 200
	repair 2200
}

atf_init_test_cases()
{
	atf_add_test_case reconnect_files
	atf_add_test_case reconnect_dirs
}
