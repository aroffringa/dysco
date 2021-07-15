#!/bin/bash
casacore_path=$1
if [[ "${casacore_path}" == "" ]] ; then
    echo Syntax: copy-to-casacore.sh /path/to/casacore
    echo That path should point to casacore\'s root directory.
else
    source_path=$(dirname "$0")/..
    dest_path=${casacore_path}/tables/Dysco
    cpp_files="
	     aftimeblockencoder.cpp
	     dyscodatacolumn.cpp dyscostman.cpp
	     dyscoweightcolumn.cpp  rftimeblockencoder.cpp rowtimeblockencoder.cpp
	     stochasticencoder.cpp threadeddyscocolumn.cpp weightencoder.cpp
	     tests/runtests.cpp tests/testbytepacking.cpp
	     tests/testdyscostman.cpp
	     tests/testtimeblockencoder.cpp"
    other_files="
	   aftimeblockencoder.h
	   bytepacker.h
	   dyscodatacolumn.h
	   dyscodistribution.h
	   dysconormalization.h
	   dyscostmancol.h
	   dyscostmanerror.h
	   dyscostman.h
	   dyscoweightcolumn.h
	   header.h
	   rftimeblockencoder.h
	   rowtimeblockencoder.h
	   serializable.h
	   stochasticencoder.h
	   threadeddyscocolumn.h
	   threadgroup.h
	   timeblockbuffer.h
	   timeblockencoder.h
	   weightblockencoder.h
	   weightencoder.h
	   LICENSE
           README.md"
    cd ${source_path}
    for f in ${cpp_files}; do
	cp -v ${f} ${dest_path}/${f%.cpp}.cc
    done
    for f in ${h_files}; do
	cp -v ${f} ${dest_path}/
    done
fi
