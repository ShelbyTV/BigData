import SCons.SConf
SCons.SConf.SetCacheMode('force')

env = Environment().Clone()
Export('env')

SConscript('source/ibf/SConscript', variant_dir='build/ibf/', duplicate=0)
SConscript('source/recs/SConscript', variant_dir='build/recs/', duplicate=0)
SConscript('source/mongoDataPrep/SConscript', variant_dir='build/mongoDataPrep/', duplicate=0)
SConscript('source/csvPrune/SConscript', variant_dir='build/csvPrune/', duplicate=0)
SConscript('scripts/SConscript', variant_dir='build/scripts/', duplicate=0)
SConscript('lib/mongo-c-driver/SConstruct', variant_dir='build/mongo-c-driver/', duplicate=0)

if GetOption("clean"):
   env.Default('.')
   Clean('.', 'build')
   Clean('.', 'config.log')
   Clean('.', '.sconf_temp')
   Clean('.', 'bin')
