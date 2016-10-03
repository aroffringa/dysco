import casacore.tables as tables

print 'tmp-compressed.ms:'
t = tables.table('tmp-compressed.ms')
print t.getdminfo()

print 'tmp-uncompressed.ms:'
t = tables.table('tmp-uncompressed.ms')
print t.getdminfo()

print 'tmp-weightcompressed.ms:'
t = tables.table('tmp-weightcompressed.ms')
print t.getdminfo()
