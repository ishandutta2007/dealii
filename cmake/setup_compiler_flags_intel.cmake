## ------------------------------------------------------------------------
##
## SPDX-License-Identifier: LGPL-2.1-or-later
## Copyright (C) 2012 - 2024 by the deal.II authors
##
## This file is part of the deal.II library.
##
## Part of the source code is dual licensed under Apache-2.0 WITH
## LLVM-exception OR LGPL-2.1-or-later. Detailed license information
## governing the source code and code contributions can be found in
## LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
##
## ------------------------------------------------------------------------

#
# General setup for the Intel C++ Compiler
#
# Please read the fat note in setup_compiler_flags.cmake prior to
# editing this file.
#

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.0" )
  message(WARNING "\n"
    "You're using an old version of the Intel C++ Compiler (icc/icpc)!\n"
    "It is strongly recommended to use at least version 19.\n"
    )
endif()


########################
#                      #
#    General setup:    #
#                      #
########################

#
# Enable verbose warnings:
#
enable_if_supported(DEAL_II_WARNING_FLAGS "-w2")

#
# Disable remarks like "Inlining inhibited by limit max-size"
#
enable_if_supported(DEAL_II_WARNING_FLAGS "-diag-disable=remark")
enable_if_supported(DEAL_II_WARNING_FLAGS "-diag-disable=16219")

#
# Disable some warnings that lead to a lot of false positives:
#
#   -w21    type qualifiers are meaningless in this declaration
#   -w68    integer conversion resulted in a change of sign
#           (triggers a lot in functionparser)
#   -w175   subscript out of range
#   -w135   class template "dealii::FE_Q_Base<POLY, dim, spacedim>"
#           has no member "Implementation"
#           (the compiler is objectively wrong since the warning
#            triggers also on code of the form
#            class FE_Q_Base {
#              struct Implementation; // forward declaration
#              friend struct Implementation;
#            };)
#   -w177   declared but not referenced
#   -w191   type qualifier is meaningless on cast type
#           Warnings from this warn about code like this:
#              static_cast<T const * const>(p)
#           There are many places in boost that do this kind of stuff
#   -w193   zero used for undefined preprocessing identifier "..."
#           This happens when using undefined preprocessor names in
#           conditions such as
#             #if (abc && def)
#           instead of
#             #if (defined(abc) && defined(def))
#           The standard says that in such cases, the undefined symbol
#           is assumed to be zero. The warning is in principle
#           useful, but the pattern appears exceedingly often in the TBB
#   -w279   controlling expression is constant
#   -w327   NULL reference is not allowed
#           (the compiler is correct here in that statements like
#            *static_cast<int*>(0) are not allowed to initialize
#            references; however, it's the only useful way to do
#            so if you need an invalid value for a reference)
#   -w383   value copied to temporary, reference to temporary used
#   -w854   const variable requires an initializer (defect 253 in http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html)
#   -w981   operands are evaluated in unspecified order
#   -w1011  missing return statement at end of non-void function
#           (incorrectly triggered by constexpr if, see #18681)
#   -w1418  external function definition with no prior declaration
#           (happens in boost)
#   -w1478  deprecation warning
#   -w1572  floating-point equality and inequality comparisons are unreliable
#   -w2259  non-pointer conversion from "double" to "float" may
#           lose significant bits
#   -w2536  type qualifiers are meaningless here
#   -w2651  attribute does not apply to any entity
#           spurious for [[deprecated]]
#   -w3415  the "always_inline" attribute is ignored on non-inline functions
#           incorrectly triggered by inline functions in tensor.h
#   -w15531 A portion of SIMD loop is serialized
#   -w15552 loop was not vectorized with "simd"
#
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd21")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd68")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd135")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd175")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd177")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd191")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd193")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd279")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd327")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd383")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd854")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd981")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd1011")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd1418")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd1478")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd1572")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd2259")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd2536")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd2651")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd3415")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd15531")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd15552")


#
# Also disable the following warnings that we frequently
# trigger writing dimension independent code:
#   -w111 statement is unreachable
#         Happens in code that is guarded by a check on 'dim'
#   -w128 loop is not reachable from preceding
#         Same as above
#   -w185 dynamic initialization in unreachable code
#         When initializing a local variable in code
#         that is executed only for one specific dimension
#   -w186 pointless comparison of unsigned integer with zero
#         The condition of for loops often depends on dim
#         and happens to evaluate to zero sometimes
#   -w280 selector expression is constant
#         When writing 'switch(dim)'
#
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd111")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd128")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd185")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd186")
enable_if_supported(DEAL_II_WARNING_FLAGS "-wd280")


#############################
#                           #
#    For Release target:    #
#                           #
#############################

if (CMAKE_BUILD_TYPE MATCHES "Release")
  #
  # General optimization flags:
  #

  add_flags(DEAL_II_CXX_FLAGS_RELEASE "-O2")

  #
  # Disable assert() in deal.II and user projects in release mode
  #
  list(APPEND DEAL_II_DEFINITIONS_RELEASE "NDEBUG")

  # equivalent to -fno-strict-aliasing:
  enable_if_supported(DEAL_II_CXX_FLAGS_RELEASE "-no-ansi-alias")

  enable_if_supported(DEAL_II_CXX_FLAGS_RELEASE "-ip")

  enable_if_supported(DEAL_II_CXX_FLAGS_RELEASE "-funroll-loops")
endif()


###########################
#                         #
#    For Debug target:    #
#                         #
###########################

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  list(APPEND DEAL_II_DEFINITIONS_DEBUG "DEBUG")

  add_flags(DEAL_II_CXX_FLAGS_DEBUG "-O0")

  enable_if_supported(DEAL_II_CXX_FLAGS_DEBUG "-g")
  enable_if_supported(DEAL_II_CXX_FLAGS_DEBUG "-gdwarf-2")
  enable_if_supported(DEAL_II_CXX_FLAGS_DEBUG "-grecord-gcc-switches")
endif()
