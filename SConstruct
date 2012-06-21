env = Environment().Clone()
Export('env')

if GetOption("clean"):

   env.Default('.')
   Clean('.', [ ".sconsign.dblite", ".sconf_temp", "config.log", "build", "bin" ])

else:

   SConscript('source/ibf/SConscript', variant_dir='build/ibf/', duplicate=0)
   SConscript('source/kMedioid/SConscript', variant_dir='build/kMedioid/', duplicate=0)
   SConscript('source/youtubeCommentsDataPrep/SConscript', variant_dir='build/youtubeCommentsDataPrep/', duplicate=0)
   SConscript('source/recs/SConscript', variant_dir='build/recs/', duplicate=0)
   SConscript('source/mongoUpdate/SConscript', variant_dir='build/mongoUpdate/', duplicate=0)
   SConscript('source/mongoDataPrep/SConscript', variant_dir='build/mongoDataPrep/', duplicate=0)
   SConscript('source/mongoCommentsGetter/SConscript', variant_dir='build/mongoCommentsGetter/', duplicate=0)
   SConscript('source/csvPrune/SConscript', variant_dir='build/csvPrune/', duplicate=0)
   SConscript('scripts/SConscript', variant_dir='build/scripts/', duplicate=0)

   # Git submodule
   SConscript('lib/mongo-c-driver/SConstruct', variant_dir='build/mongo-c-driver/', duplicate=0)

   # Other library
#   SConscript('lib/compact-language-detector/SConscript', variant_dir='build/compact-language-detector/', duplicate=0)


