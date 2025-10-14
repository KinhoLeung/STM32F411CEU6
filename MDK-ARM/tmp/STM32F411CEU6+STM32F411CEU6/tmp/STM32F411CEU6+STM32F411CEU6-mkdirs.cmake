# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6")
  file(MAKE_DIRECTORY "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6")
endif()
file(MAKE_DIRECTORY
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/1"
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6"
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/tmp"
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/src/STM32F411CEU6+STM32F411CEU6-stamp"
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/src"
  "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/src/STM32F411CEU6+STM32F411CEU6-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/src/STM32F411CEU6+STM32F411CEU6-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/Desktop/STM32F411CEU6 - 副本/MDK-ARM/tmp/STM32F411CEU6+STM32F411CEU6/src/STM32F411CEU6+STM32F411CEU6-stamp${cfgdir}") # cfgdir has leading slash
endif()
