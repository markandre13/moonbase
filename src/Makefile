# Generated automatically from Makefile.in by configure.
##############################################################################
#                                                                            #
# MoonBase - a dBASE compatible database for C++                             #
# makefile for 'libdbase.a' and 'moonbase'                                   #
# (C)opyright 1995 by Mark-André Hopf                                        #
# (C)opyright 1995 by MAGIC INCLUDED RESEARCH                                #
#                                                                            #
# dBASE is a (tm) of Borland International, Inc.                             #
#                                                                            #
##############################################################################

CC 				= c++

#CFLAGS		= -frtti -fhandle-exceptions -Wall -Wwrite-strings -Woverloaded-virtual -O2
CFLAGS		= -Wall -Wwrite-strings -Woverloaded-virtual -ggdb

DEFINES		= -DHAVE_CONFIG_H

PRGFILE		= moonbase
PRGOBJS		= frontend.o

LIBFILE		=	libdbase.a
LIBOBJS		=	moonbase.o dbdata.o dbfileio.o dbheader.o dbmemoio.o dblcklst.o

INCDIRS		= -I/usr/lib/g++ -I../include
LIBDIRS		= -L.

LIBRARIES	= -ldbase

all::$(PRGFILE)

#---------------------------------------------------------------------------
# compiling
#---------------------------------------------------------------------------
.SUFFIXES: .cc

.cc.o:
	@echo compiling $*.cc ...
	@$(CC) $(CFLAGS) $(DEFINES) $(INCDIRS) $*.cc -c -o $*.o

#---------------------------------------------------------------------------
# building the library
#---------------------------------------------------------------------------
$(LIBFILE): $(LIBOBJS)
	@echo updating library $(LIBNAME)
	@ar r $(LIBFILE) $(LIBOBJS)

#---------------------------------------------------------------------------
# linking
#---------------------------------------------------------------------------
$(PRGFILE): $(PRGOBJS) $(LIBFILE) 
	@echo linking $(PRGFILE) ...
	@$(CC) $(PRGOBJS) -o $(PRGFILE) $(LIBDIRS) $(LIBRARIES)
	@echo Ok
