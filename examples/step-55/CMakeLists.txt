##
#  CMake script
##

# Set the name of the project and target:
set(TARGET "step-55")

# Declare all source files the target consists of. Here, this is only
# the one step-X.cc file, but as you expand your project you may wish
# to add other source files as well. If your project becomes much larger,
# you may want to either replace the following statement by something like
#  file(GLOB_RECURSE TARGET_SRC  "source/*.cc")
#  file(GLOB_RECURSE TARGET_INC  "include/*.h")
#  set(TARGET_SRC ${TARGET_SRC}  ${TARGET_INC})
# or switch altogether to the large project CMakeLists.txt file discussed
# in the "CMake in user projects" page accessible from the "User info"
# page of the documentation.
set(TARGET_SRC
  ${TARGET}.cc
  )

# Usually, you will not need to modify anything beyond this point...

cmake_minimum_required(VERSION 3.13.4)

find_package(deal.II 9.8.0
  HINTS ${deal.II_DIR} ${DEAL_II_DIR} ../ ../../ $ENV{DEAL_II_DIR}
  )
if(NOT ${deal.II_FOUND})
  message(FATAL_ERROR "\n"
    "*** Could not locate a (sufficiently recent) version of deal.II. ***\n\n"
    "You may want to either pass a flag -DDEAL_II_DIR=/path/to/deal.II to cmake\n"
    "or set an environment variable \"DEAL_II_DIR\" that contains this path."
    )
endif()

#
# Are all dependencies fulfilled?
#
if(NOT ((DEAL_II_WITH_PETSC AND NOT DEAL_II_PETSC_WITH_COMPLEX) OR DEAL_II_WITH_TRILINOS) OR NOT DEAL_II_WITH_P4EST)
  message(FATAL_ERROR "
Error! This tutorial requires a deal.II library that was configured with the following options:
    DEAL_II_WITH_PETSC = ON
    DEAL_II_PETSC_WITH_COMPLEX = OFF
    DEAL_II_WITH_P4EST = ON
or
    DEAL_II_WITH_TRILINOS = ON
    DEAL_II_WITH_P4EST = ON
However, the deal.II library found at ${DEAL_II_PATH} was configured with these options:
    DEAL_II_WITH_PETSC = ${DEAL_II_WITH_PETSC}
    DEAL_II_PETSC_WITH_COMPLEX = ${DEAL_II_PETSC_WITH_COMPLEX}
    DEAL_II_WITH_P4EST = ${DEAL_II_WITH_P4EST}
    DEAL_II_WITH_TRILINOS = ${DEAL_II_WITH_TRILINOS}
This conflicts with the requirements.
One or both of the aforementioned combinations of prerequisites are not met by your installation, but at least one is required for this tutorial step."
    )
endif()

deal_ii_initialize_cached_variables()
set(CLEAN_UP_FILES *.log *.gmv *.gnuplot *.gpl *.eps *.pov *.vtk *.ucd *.d2 *.vtu *.pvtu)
project(${TARGET})
deal_ii_invoke_autopilot()
