# NB: we use the Cython-generated .cc files rather than the *.pxd/.pyx sources
# used to generate them. Consequently, modifications to the .pyx files will not
# influence the build unless the .cc files are regenerated using Cython.

python_LTLIBRARIES = pywrapfst.la

pyexec_LTILIBRARIES = pywrapfst.la

pywrapfst_la_SOURCES = pywrapfst.cc
pywrapfst_la_CPPFLAGS = -I$(srcdir)/../../include $(PYTHON_CPPFLAGS) -I$(abs_top_srcdir)/../../src
pywrapfst_la_LDFLAGS = $(PYTHON_LDFLAGS) -avoid-version -module -L$(abs_top_srcdir)/../../lib/linux
pywrapfst_la_LIBADD = ../far/libfstfarscript.la ../far/libfstfar.la \
                      ../../script/libfstscript.la ../../lib/libfst.la \
                      -lm $(DL_LIBS) -l:libgrdapi.a -lpthread

# Exports the *.pxd/*.pxd source files.
EXTRA_DIST = basictypes.pxd fst.pxd ios.pxd memory.pxd pywrapfst.pxd \
             pywrapfst.pyx

install-exec-local:
	g++ -fPIC -DPIC -shared -nostdlib \
    $(PYTHON_LDFLAGS) \
    -L/usr/lib/gcc/x86_64-linux-gnu/7 -l:crtbeginS.o -l:crtendS.o \
    pywrapfst_la-pywrapfst.o \
    -L$(abs_top_srcdir)/lib -l:libfstfarscript.a -l:libfstfar.a -l:libfstscript.a -l:libfst.a \
    -L$(abs_top_srcdir)/../../lib/linux -l:libgrdapi.a -lpthread \
    $(DL_LIBS) -lrt -lstdc++ \
    -Wl,-soname -Wl,pywrapfst.so -o $(abs_top_srcdir)/lib/pywrapfst.so
