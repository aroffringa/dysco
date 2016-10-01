# dysco
A compressing storage manager for Casacore mearement sets

To install:

    mkdir build
    cd build
    cmake ../
    make -j 4
    make install

To be able to open compressed measurement sets, the dysco library ("libdyscostman.so") must be in your path.

## File format compatibility
I have changed the file format of Dysco on 2016-10-01, which means that observations packed with the older
DyscoStMan can not be opened by the current version. This change was needed to insert a missing keyword in the file.
I've carefully designed the new file so that no more incompatible changes are necessary; i.e., the file format is
fixed now, and all future version of Dysco will be able to open the current files.

## dscompress
The program 'dscompress' allows compression of a measurement set. Note that this program is mostly aimed at testing the compression.
For production use I highly recommend to perform compression within a pipeline in which the initial measurement sets are created.
This can be implemented by asking Casacore to store a column with the DyscoStMan. For LOFAR, Dysco was implemented in DPPP, and
using DPPP is the recommended way to compress a LOFAR measurement set.

Run 'dscompress' without parameters to get help. Here's an example command:

    dscompress -afnormalization -truncgaus 2.5 -fit-to-maximum -bits-per-float 4 \
       -bits-per-weight 12 -column DATA -column WEIGHT_SPECTRUM obs.ms

It might be that this command does not decrease the size of the measurement set because, depending on the old storage manager,
Casacore might not clear up the space of the old column after replacing it. Hence, the above statement is useful for testing the
effects of compression, but not to see the size of the measurement set.

To be able to get a measure of the size, one can add '-reorder' to the command line. This will free up the space of the old column,
but currently in order to do so, the full measurement set will be rewritten. This causes the compression to be performed twice,
and hence the compression noise will be added twice. Therefore, the '-reorder' option is useful for testing the size of the compression,
but should not be used in production.
