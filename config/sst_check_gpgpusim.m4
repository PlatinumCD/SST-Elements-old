AC_DEFUN([SST_CHECK_GPGPUSIM],
[
   sst_check_gpgpusim_happy="no"

   AC_ARG_WITH([gpgpusim],
      [AS_HELP_STRING([--with-gpgpusim@<:@=DIR@:>@],
         [Specify the root directory for GPGPU-Sim])])

   CPPFLAGS_saved="$CPPFLAGS"
   LDFLAGS_saved="$LDFLAGS"
   LIBS_saved="$LIBS"

   #Need compiler versions
   CC_VERSION=$(gcc -dumpversion)
   CUDA_VERSION_STRING=$(nvcc --version | grep -o "release .*" | sed 's/ *,.*//' | sed 's/release //g' | sed 's/\./ /g' | awk '{printf("%02u%02u", 10*int(@S|@1), 10*@S|@2);}')
   GPGPUSIM_LIB_DIR=lib/gcc-$CC_VERSION/cuda-$CUDA_VERSION_STRING/release

   AS_IF([test ! -z "$with_gpgpusim" -a "$with_gpgpusim" != "yes"],
         [GPGPUSIM_CPPFLAGS=""
            CPPFLAGS="$GPGPUSIM_CPPFLAGS $CPPFLAGS"
            GPGPUSIM_LIBDIR="$with_gpgpusim/$GPGPUSIM_LIB_DIR"
            GPGPUSIM_LDFLAGS="-L$GPGPUSIM_LIBDIR"
            LDFLAGS="$GPGPUSIM_LDFLAGS $LDFLAGS"],
         [GPGPUSIM_CPPFLAGS=
            GPGPUSIM_LDFLAGS=
            GPGPUSIM_LIBDIR=
            GPGPUSIM_LIB=
   ])

   AS_IF([test "$sst_check_gpgpusim_happy" = "no"],
      [],
      [
      AS_IF([test "$sst_check_cuda_happy" = "no"],
            [AC_MSG_ERROR([Valid Cuda required for GPGPU-Sim])])
   ])

   AC_MSG_CHECKING([for cudart_mod in $GPGPUSIM_LIBDIR])
      if test -f "$GPGPUSIM_LIBDIR/libcudart_mod.so" ; then
         GPGPUSIM_LIB="-lcudart_mod"
         sst_check_gpgpusim_happy="yes"
         AC_MSG_RESULT([yes])
      else
         AC_MSG_RESULT(libcudart_mod.so not found)
      fi

   ##TODO
   #This is a better test but fails because it must be linked against the SST GPU component
   #AC_LANG_PUSH(C++)
   #AC_CHECK_LIB([cudart_mod], [cudaLaunch],
      #[GPGPUSIM_LIB="-lcudart_mod"], [sst_check_gpgpusim_happy="no"]
   #)
   #AC_LANG_POP(C++)

   CPPFLAGS="$CPPFLAGS_saved"
   LDFLAGS="$LDFLAGS_saved"
   LIBS="$LIBS_saved"

   AC_SUBST([GPGPUSIM_CPPFLAGS])
   AC_SUBST([GPGPUSIM_LDFLAGS])
   AC_SUBST([GPGPUSIM_LIBDIR])
   AC_SUBST([GPGPUSIM_LIB])

   AS_IF([test "$sst_check_gpgpusim_happy" = "yes"],
         [AC_DEFINE_UNQUOTED([GPGPUSIM_LIBDIR], ["$GPGPUSIM_LIBDIR"], [Library directory where GPGPU-Sim can be found.])])
   AS_IF([test "$sst_check_gpgpusim_happy" = "yes"], [$1], [$2])

])
