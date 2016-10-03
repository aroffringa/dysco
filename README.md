# dysco
A compressing storage manager for Casacore mearement sets

To install:

    mkdir build
    cd build
    cmake ../
    make -j 4
    make install

To be able to open compressed measurement sets, the dysco library ("libdyscostman.so") must be in your path.

The Dysco compression technique is explained in the article "Compression of interferometric radio-astronomical data",
A. R. Offringa (2016; http://arxiv.org/abs/1609.02019).

## File format compatibility
I have changed the file format of Dysco on 2016-10-01, which means that observations packed with the older
DyscoStMan can not be opened by the current version. This change was needed to insert a missing keyword in the file.
I've carefully designed the new file format so that no more incompatible changes are necessary; i.e., the file format is
fixed now, and all future versions of Dysco will be able to open the current files.

Further documentation for the storage manager, including documentation of the `dscompress` tool:

https://github.com/aroffringa/dysco/wiki
