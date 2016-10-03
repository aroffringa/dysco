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

## dscompress
The program 'dscompress' allows compression of a measurement set. Note that this program is mostly aimed at testing the compression.
For production use I highly recommend to perform compression within a pipeline in which the initial measurement sets are created.
This can be implemented by asking Casacore to store a column with the DyscoStMan. For LOFAR, Dysco was implemented in DPPP, and
using DPPP is the recommended way to compress a LOFAR measurement set.

Run 'dscompress' without parameters to get help. Here's an example command:

    dscompress -afnormalization -truncgaus 2.5 -fit-to-maximum -bits-per-float 4 \
       -bits-per-weight 12 -column DATA -column WEIGHT_SPECTRUM obs.ms

It might be that this command does not decrease the size of the measurement set because, depending on the previously used storage
manager, Casacore might not clear up the space of the old column after replacing it. Hence, the above statement is useful for testing
the effects of compression, but not to see the size of the measurement set.

To be able to get a measure of the size, one can add '-reorder' to the command line. This will free up the space of the old column,
but currently in order to do so, the full measurement set will be rewritten. This causes the compression to be performed twice,
and hence the compression noise will be added twice. Therefore, the '-reorder' option is useful for testing the size of the compression,
but should not be used in production.

## Parameters
The Casacore system pass storage manager parameters to the storage manager through a so-called "Spec" Record. For Dysco, this Record
can hold the following fields:

 * dataBitCount
 
   An integer, representing the number of bits used for each float in a data column, such as DATA or CORRECTED_DATA. Note that each
   visibility contains two floats (the real and imaginary). It must be set to a proper value even if only a weight column is compressed.
   Typical values for this are 4-12 bits, which compresses the data by a factor from 16 to 2.7.
   
   Note that Dysco currently only supports a particular set of bit counts: 2, 3, 4, 6, 8, 10, 12 and 16.
   
 * weightBitCount
 
   An integer, the number of bits used for each float in the WEIGHT_SPECTRUM column. Note that Dysco will use a single weight for all
   polarizations, so Dysco will typically compress the weight volume already by a factor of 4. Using 12 bits is very safe and
   changes the weights insignificantly. Lower bit counts are possible, but because 12 bits compresses the weights already by
   a factor of 11, the resulting weight volume from 12 bit compression is often factors lower than the data column volume.
   
   As for the data bit count, the supported settings are 2, 3, 4, 6, 8, 10, 12 and 16 bits.
   
 * distribution

   A string specifying the distribution assumed for the data. Supported values are:
   
   + Gaussian
   + Uniform
   + StudentsT
   + TruncatedGaussian
   
   Generally, the TruncatedGaussian distribution delivers the best results, but the Uniform distribution is a closed second.
   Gaussian and StudentsT are not recommended.

 * normalization
 
   A string specifying how to normalize the data before quantization. This can be either "AF", "RF" or "Row". AF means
   Antenna-Frequency normalization, RF means Row-Frequency normalization and Row means per-row normalization. Row normalization
   is not stable and is *not* recommended. AF and RF normalization produce very similar results. For reasonably noise data,
   AF is the recommended method. In high SNR cases, RF normalization might be better.
 
 * studentTNu
 
   When using the StudentsT distribution, this value specifies the degrees of freedom of the distribution.
 
 * distributionTruncation
 
   When using the TruncatedGaussian distribution, this value specifies where to truncate the distribution, relative to the standard
   deviation. In Offringa (2016), values of 1.5, 2.5 and 3.5 were tested, and 2.5 seemed to be generally best, in particular
   in combination with AF normalization. With RF normalization, 1.5 was slightly more accurate.

All fields are case sensitive.
