cmake_minimum_required(VERSION 2.8.3)

project(pico_reco)

# set cpp 14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify additional locations of header files
include_directories(
  include
  SYSTEM
)

###########
## Build ##
###########

# set build type
set(CMAKE_BUILD_TYPE Release)	

# Make primary shared library
add_library(pico_reco SHARED
  src/pico_reco.cpp
  src/jhcPicoReco.cpp
)
set_target_properties(pico_reco PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Make auxilliary sample program "test_reco"
add_executable(test_reco
  src/test_reco.cpp
  src/jhcPicoReco.cpp
)
set_target_properties(test_reco PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# requires Raspbian Bullseye for correct GLIBC version
# Note: these library paths are "baked into" resulting shared lib
#       copy appropriate version to /usr/lib if built somewhere else
set(pico_reco_LIB
#  ${PROJECT_SOURCE_DIR}/lib_jet/libpv_cheetah.so  # Cortex a57 64 bit
  ${PROJECT_SOURCE_DIR}/lib_rpi/libpv_cheetah.so  # Cortex a72 32 bit
  libasound.so
  pthread
)
target_link_libraries(pico_reco
  ${pico_reco_LIB}
)
target_link_libraries(test_reco
  ${pico_reco_LIB}
)

