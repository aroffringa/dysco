#!/bin/bash

# Copyright (C) 2021 ASTRON (Netherlands Institute for Radio Astronomy)
# SPDX-License-Identifier: GPL-3.0-or-later

#Script configuration for this repo. Adjust it when copying to a different repo.

#The directory that contains the source files.
SOURCE_DIR=$(dirname "$0")/..

#Directories that must be excluded from formatting. These paths are
#relative to SOURCE_DIR.
EXCLUDE_DIRS=(build CMake)

#The patterns of the C++ source files, which clang-format should format.
CXX_SOURCES=(*.cpp *.h)

#The patterns of the CMake source files, which cmake-format should format.
CMAKE_SOURCES=(CMakeLists.txt *.cmake)

#End script configuration.

#The common formatting script has further documentation.
source $(dirname "$0")/format.sh
