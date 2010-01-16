
Version 1.41.0

New Libraries

     * Property Tree: A tree data structure especially suited to storing configuration data, from Marcin Kalicinski and Sebastian Redl.

Updated Libraries

     * DateTime:
          + The default format for time durations is now "%-%O:%M:%S%F" instead of "%-%H:%M:%S%F" that was used previously. In order to retain the old behavior, the format string has to be specified explicitly during the time IO facet construction (#1861).
          + Gregorian dates now use 32-bit integer type internally on 64-bit platforms (#3308).
          + See the full changelog for more detail.
     * Filesystem:
          + Bug fixes: (#3385). (#3528). (#3509).
     * Iostreams:
          + Add a grep filter (#1627).
          + Support archives with multiple members (#1896).
          + Make tee work with input streams (#791).
          + Improved filesystem interoperability.
          + Several warnings fixed or suppressed (including #1618, #1875, #2779).
          + Various other fixes (including #1580, #1671).
     * Math: Substantially improved the performance of the incomplete gamma function and it's inverse: this enhances the performance of the gamma, poisson, chi-squared and non-central chi-squared distributions.
     * Multi-index Containers: Maintenance fixes. Consult the library release notes for further information.
     * Proto:
          + Clean up some MSVC warnings and errors in /Za (disable Microsoft extensions) mode.
          + Fixes for c++0x mode on various compilers.
     * Python: Boost.Python now supports Python 3 (Haoyu Bai's Google Summer of Code project, mentored by Stefan Seefeld).
     * Regex: Added support for many Perl 5.10 syntax elements including named sub-expressions, branch resets and recursive regular expressions.
     * Spirit: This is the initial official release of the new Spirit V2.1, a completely new library for parsing, lexing, and output generation. Note: this release is not backwards compatible with earlier versions.
     * System:
          + Bug fix: (#3559).
     * Thread:
          + Support for futures, promises and packaged tasks added
          + boost::thread_specific_ptr is now faster when there are lots of thread-specific objects
          + Some Boost.Thread facilities are now header-only
     * Unordered: Major update:
          + Replaced a lot of the macro based implementation with a cleaner template based implementation.
          + Reduced memory use.
          + Full details in the changelog.
     * Utility: A "const" issue of value_initialized is fixed: Its data() member function and its conversion operator are replaced by overloads for const and non-const access (#2548).
     * Wave: See the changelog for details.
     * Xpressive:
          + Fix infinite loop with some uses of \Q...\E quotemeta (#3586).
          + Eliminate unreachable code warnings on MSVC
          + Clean up some MSVC warnings and errors in /Za ("disable Microsoft extensions") mode.
          + Fixes for c++0x mode on various compilers.

Build System

   A bug preventing "fat" 32-bit + 64-bit builds on OSX has been fixed.

Boost.CMake moved

   The cmake version of boost has moved; the Boost.CMmake release will be separate and will lag the main release slightly, but will also be capable of producing patch releases as necessary.

   More information on the Boost CMake wiki page.

Updated Tools

     * Quickbook 1.5: These changes require your document to use the [quickbook 1.5] tag:
          + More intuitive syntax and variable lookup for template calls (#1174, #2034, #2036).
          + Image attributes (#1157)
          + Table Ids (#1194)
          + Better handling of whitespace in section syntax. (#2712)

Compilers Tested

   Boost's primary test compilers are:
     * OS X:
          + GCC 4.0.1 on Intel Tiger and Leopard.
          + GCC 4.0.1 on PowerPC Tiger.
     * Linux:
          + GCC 4.4.1 on Ubuntu Linux.
          + GCC 4.4 on Debian
     * Windows:
          + Visual C++ 7.1 SP1, 8.0 SP1 and 9.0 SP1 on Windows XP.

   Boost's additional test compilers include:
     * Linux:
          + Intel 10.1 on Red Hat Enterprise Linux.
          + Intel 10.1 on 64-bit Red Hat Enterprise Linux.
          + Intel 10.1 on Suse Linux on 64 bit Itanium.
          + Intel 11.0 on 32 bit Red Hat Enterprise Linux.
          + Intel 11.0 on 64 bit Red Hat Enterprise Linux.
          + Intel 11.1 on 64 bit Red Hat Enterprise Linux.
          + Intel 11.1 on 64 bit Linux Redhat 5.1 Server.
          + GCC 3.4.3, GCC 4.2.4, GCC 4.3.3 and GCC 4.4.1 on Red Hat Enterprise Linux.
          + GCC 4.3.3 and GCC 4.4.1 with C++0x extensions on Red Hat Enterprise Linux.
          + GCC 4.3.3 on 64-bit Redhat Server 5.1.
          + GCC 4.3.3 on 64 bit Linux.
          + GCC 4.3.4 on Debian unstable.
          + GCC 4.3.2 on 64 bit Gentoo.
          + QLogic PathScale(TM) Compiler Suite: Version 3.2 on Red Hat Enterprise Linux.
          + Sun 5.9 on Red Hat Enterprise Linux.
     * OS X:
          + Intel C++ Compiler 11.1 on Leopard.
          + Intel C++ Compiler 10.1, 11.0.
          + GCC 4.0.1 on Intel Tiger.
          + GCC 4.0.1 on PowerPC Tiger.
     * Windows:
          + Visual C++ 7.1, 8,0, 9,0 on XP.
          + Visual C++ 9.0 on 32-bit Vista.
          + Visual C++ 9.0 on AMD 64-bit Vista.
          + Visual C++ 9.0 using STLport 5.2 on XP and Windows Mobile 5.0.
          + Visual C++ 10.0 beta 1 with a patch for the program options lib.
          + Borland/Codegear C++ 5.9.3, 6.1.3 (2009), 6.2.0 (2010).
          + Intel C++ 11.1, with a Visual C++ 9.0 backend, on Vista 32-bit.
          + GCC 4.4.1 on Mingw, with and without C++0x extensions.
     * AIX:
          + IBM XL C/C++ Enterprise Edition for AIX, V10.1.0.0, on AIX Version 5.3.0.40.
     * FreeBSD:
          + GCC 4.2.1 on FreeBSD 7.0.
     * Solaris:
          + Sun C++ 5.10 on Solaris 5.10.

Acknowledgements

   Beman Dawes, Eric Niebler, Rene Rivera, and Daniel James managed this release.
