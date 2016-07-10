# $FreeBSD$

SSP_CFLAGS=

FILES+=	${.CURDIR}/../common/crt.export

SED_FIX_NOTE = -i "" -e '/\.note\.tag/s/progbits/note/'

NO_WMISSING_VARIABLE_DECLARATIONS=

.include "../Makefile.inc"
