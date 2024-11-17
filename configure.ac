AC_INIT([macer], [0.2], [lee@cifro.codes], [macer], [https://code.cifro.codes/macer])
AM_INIT_AUTOMAKE([foreign subdir-objects dist-bzip2 dist-xz])

AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

AC_MSG_CHECKING([Gcc variadic macro comma support])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    void foo(int, int) {}
    #define FOO(...) foo(0, 1, ## __VA_ARGS__)
    void bar() { FOO(); }
  ]])], AC_MSG_RESULT([yes]), AC_MSG_ERROR([failed])
)

AC_MSG_CHECKING([c++11 enable-if type traits])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[#include <type_traits> typename std::enable_if<true>::type foo();]])], AC_MSG_RESULT([yes]), AC_MSG_ERROR([failed])
)

AC_MSG_CHECKING([c++11 variadic template support])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[template<typename... T> void foo(T...) {}]])], AC_MSG_RESULT([yes]), AC_MSG_ERROR([failed])
)

AC_CHECK_HEADER([endian.h], [], AC_MSG_ERROR([Unable to find endianness header]))
AC_MSG_CHECKING([Little endian processor])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[#include <endian.h> static_assert(BYTE_ORDER == LITTLE_ENDIAN, "endianess");]])], AC_MSG_RESULT([yes]), AC_MSG_ERROR([failed])
)

AC_CHECK_HEADER([libusb-1.0/libusb.h], [], AC_MSG_ERROR([Unable to find libusb header]))
AC_SEARCH_LIBS([pthread_create], [pthread], [], AC_MSG_ERROR([Unable to find pthread library]))
AC_SEARCH_LIBS([libusb_init], [usb-1.0], [], AC_MSG_ERROR([Unable to find libusb library]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
