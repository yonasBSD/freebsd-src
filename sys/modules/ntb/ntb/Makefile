.PATH:  ${SRCTOP}/sys/dev/ntb

KMOD    = ntb
SRCS    = ntb.c ntb_if.c
SRCS += device_if.h bus_if.h ntb_if.h

EXPORT_SYMS=	YES

.include <bsd.kmod.mk>
