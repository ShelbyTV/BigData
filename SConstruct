env = Environment()
Export('env')
SConscript('source/ibf/SConscript', variant_dir='build/ibf/', duplicate=0)
SConscript('source/recs/SConscript', variant_dir='build/recs/', duplicate=0)
SConscript('source/mongoDataPrep/SConscript', variant_dir='build/mongoDataPrep/', duplicate=0)
SConscript('source/csvPrune/SConscript', variant_dir='build/csvPrune/', duplicate=0)
Clean('.', 'build')
Clean('.', 'bin')
