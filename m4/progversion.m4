# progversion.m4
dnl Copyright (C) 2008  Giel van Schijndel
dnl Copyright (C) 2008  Warzone Resurrection Project
dnl
dnl This file is free software; I, Giel van Schijndel give unlimited
dnl permission to copy and/or distribute it, with or without modficiations,
dnl as long as this notice is preserved.

AC_PREREQ(2.50)

AC_DEFUN([AC_PROG_VERSION_CHECK],
[
[
	ac_prog_version_$1=`$1 --version | head -n 1 | sed 's/([^)]*)//g;s/^[a-zA-Z\.\ \-\/]*//;s/ .*$//'`
	ac_prog_major_$1=`echo $ac_prog_version_$1 | cut -d. -f1`
	ac_prog_minor_$1=`echo $ac_prog_version_$1 | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
	ac_prog_micro_$1=`echo $ac_prog_version_$1 | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
	[ -z "$ac_prog_minor_$1" ] && ac_prog_minor_$1=0
	[ -z "$ac_prog_micro_$1" ] && ac_prog_micro_$1=0

	ac_prog_min_major_$1=`echo $2 | cut -d. -f1`
	ac_prog_min_minor_$1=`echo $2 | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
	ac_prog_min_micro_$1=`echo $2 | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
	[ -z "$ac_prog_min_minor_$1" ] && ac_prog_min_minor_$1=0
	[ -z "$ac_prog_min_micro_$1" ] && ac_prog_min_micro_$1=0
]

	AC_MSG_CHECKING([for $1 >= $2])

[
	if [ "$ac_prog_major_$1" -lt "$ac_prog_min_major_$1" ]; then
		ac_prog_wrong_$1=1
	elif [ "$ac_prog_major_$1" -eq "$ac_prog_min_major_$1" ]; then
		if [ "$ac_prog_minor_$1" -lt "$ac_prog_min_minor_$1" ]; then
			ac_prog_wrong_$1=1
		elif [ "$ac_prog_minor_$1" -eq "$ac_prog_min_minor_$1" -a "$ac_prog_micro_$1" -lt "$ac_prog_min_micro_$1" ]; then
			ac_prog_wrong_$1=1
		fi
	fi

	if [ ! -z "$ac_prog_wrong_$1" ]; then
]
		AC_MSG_ERROR([found $ac_prog_version_$1, not ok])
	else
		AC_MSG_RESULT([found $ac_prog_version_$1, ok])
	fi

	ifelse([$3], [], , [
		for version in $3; do
		[
			ac_prog_not_major_$1=`echo $version | cut -d. -f1`
			ac_prog_not_minor_$1=`echo $version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
			ac_prog_not_micro_$1=`echo $version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
			[ -z "$ac_prog_not_minor_$1" ] && ac_prog_not_minor_$1=0
			[ -z "$ac_prog_not_micro_$1" ] && ac_prog_not_micro_$1=0
		]
			AC_MSG_CHECKING([for $1 != $version])
		[
			if [ "$ac_prog_major_$1" -eq "$ac_prog_not_major_$1" -a "$ac_prog_minor_$1" -eq "$ac_prog_not_minor_$1" -a "$ac_prog_micro_$1" -eq "$ac_prog_not_micro_$1" ]; then
		]
				AC_MSG_ERROR([found $ac_prog_version_$1, not ok])
			else
				AC_MSG_RESULT([not found, good])
			fi
		done
	])
])
