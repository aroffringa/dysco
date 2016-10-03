ms=$1

if [[ "${ms}" == "" ]] ; then
    echo Syntax: testndppp.sh \<input.ms\>
else
    echo Removing temporary files...
    rm -rf tmp-compressed.ms tmp-uncompressed.ms tmp-weightcompressed.ms
    
    echo Copying to uncompressed...
    ${NDP} NDPPP-input-to-uncompressed.parset msin=${ms}
    if [ $? != 0 ] ; then exit ; fi
    
    echo Making compressed...
    ${NDP} NDPPP-uncompressed-to-compressed.parset
    if [ $? != 0 ] ; then exit ; fi
    
    echo Adding compressed column to uncompressed...
    ${NDP} NDPPP-uncompressed-add-compressed-column.parset
    if [ $? != 0 ] ; then exit ; fi
    
    echo Adding compressed column to compressed...
    ${NDP} NDPPP-compressed-add-compressed-column.parset
    if [ $? != 0 ] ; then exit ; fi
    
    echo Adding uncompressed column to uncompressed...
    ${NDP} NDPPP-uncompressed-add-uncompressed-column.parset
    if [ $? != 0 ] ; then exit ; fi
    
    echo Adding uncompressed column to compressed...
    ${NDP} NDPPP-compressed-add-uncompressed-column.parset
    if [ $? != 0 ] ; then exit ; fi
    
    echo Making weight compressed...
    ${NDP} NDPPP-uncompressed-to-weightcompressed.parset
    if [ $? != 0 ] ; then exit ; fi

    python showdminfo.py
    
    echo Removing temporary files...
    rm -rf tmp-compressed.ms tmp-uncompressed.ms tmp-weightcompressed.ms
fi
