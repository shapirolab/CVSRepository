# fcp make
SHELL = /bin/csh
GCC = gcc
CFLAGS = -c -DDEBUG -g -DULTRIX -DLINUX

INCLUDES = -I.

OFILES = cnv.o concatenate.o ctl.o dist.o emulate.o externs.o fcp.o \
	 file.o freeze_term.o freezer.o global.o heap.o interface.o \
	 kernels.o link_static.o logix.o math.o notify.o spicomm.o \
	 spiweight.o streams.o timer.o tty.o unify.o utility.o


LIBS   = -lm -lc

fcp: \
	$(OFILES)
	$(GCC) $(OFILES) $(LIBS) -o fcp

psiweight.o: \
	psiweight.c
	$(GCC) $(CFLAGS) $(OPT) $(INCLUDES) psiweight.c

