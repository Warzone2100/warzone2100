# progversion.m4
dnl Copyright (C) 2008  Giel van Schijndel
dnl Copyright (C) 2008-2009  Warzone Resurrection Project
dnl
dnl This file is free software; I, Giel van Schijndel give unlimited
dnl permission to copy and/or distribute it, with or without modficiations,
dnl as long as this notice is preserved.

AC_PREREQ(2.50)

AC_DEFUN([AC_PROG_VERSION_CHECK],
[
[
	for ac_prog in $1; do
		ac_prog_version_check=`$ac_prog --version | head -n 1 | sed 's/([^)]*)//g;s/^[-a-zA-Z\.\ \/]*//;s/ .*$//'`
		ac_prog_major_check=`echo $ac_prog_version_check | cut -d. -f1`
		ac_prog_minor_check=`echo $ac_prog_version_check | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
		ac_prog_micro_check=`echo $ac_prog_version_check | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
		[ -z "$ac_prog_minor_check" ] && ac_prog_minor_check=0
		[ -z "$ac_prog_micro_check" ] && ac_prog_micro_check=0

		ac_prog_min_major_check=`echo $2 | cut -d. -f1`
		ac_prog_min_minor_check=`echo $2 | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
		ac_prog_min_micro_check=`echo $2 | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
		[ -z "$ac_prog_min_minor_check" ] && ac_prog_min_minor_check=0
		[ -z "$ac_prog_min_micro_check" ] && ac_prog_min_micro_check=0
]

		AC_MSG_CHECKING([for $ac_prog >= $2])

[
		if [ "$ac_prog_major_check" -lt "$ac_prog_min_major_check" ]; then
			ac_prog_wrong_check=1
		elif [ "$ac_prog_major_check" -eq "$ac_prog_min_major_check" ]; then
			if [ "$ac_prog_minor_check" -lt "$ac_prog_min_minor_check" ]; then
				ac_prog_wrong_check=1
			elif [ "$ac_prog_minor_check" -eq "$ac_prog_min_minor_check" -a "$ac_prog_micro_check" -lt "$ac_prog_min_micro_check" ]; then
				ac_prog_wrong_check=1
			fi
		fi

		if [ ! -z "$ac_prog_wrong_check" ]; then
]
			AC_MSG_ERROR([found $ac_prog_version_check, not ok])
		else
			AC_MSG_RESULT([found $ac_prog_version_check, ok])
		fi

		ifelse([$3], [], , [
			for version in $3; do
[
				ac_prog_not_major_check=`echo $version | cut -d. -f1`
				ac_prog_not_minor_check=`echo $version | sed s/[-,a-z,A-Z].*// | cut -d. -f2`
				ac_prog_not_micro_check=`echo $version | sed s/[-,a-z,A-Z].*// | cut -d. -f3`
				[ -z "$ac_prog_not_minor_check" ] && ac_prog_not_minor_check=0
				[ -z "$ac_prog_not_micro_check" ] && ac_prog_not_micro_check=0
]
				AC_MSG_CHECKING([for $ac_prog != $version])
[
				if [ "$ac_prog_major_check" -eq "$ac_prog_not_major_check" -a "$ac_prog_minor_check" -eq "$ac_prog_not_minor_check" -a "$ac_prog_micro_check" -eq "$ac_prog_not_micro_check" ]; then
]
					AC_MSG_ERROR([found $ac_prog_version_check, not ok])
				else
					AC_MSG_RESULT([not found, good])
				fi
			done
		])
	done
])
