# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION='0.0.1'
APPNAME='ndn-wifidirect-linux'

def options(opt):
    opt.load('compiler_cxx default-compiler-flags')

def configure(conf):
    conf.load("compiler_cxx default-compiler-flags")

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', global_define=True, mandatory=True)

def build (bld):
    bld(target='daemon',
        features=['cxx', 'cxxprogram'],
        source='daemon.cpp',
        use='NDN_CXX')

    if len(bld.path.ant_glob('daemonGO.cpp')):
	    bld(target='daemonGO',
    	    features=['cxx', 'cxxprogram'],
        	source='daemonGO.cpp',
        	use='NDN_CXX')

    if len(bld.path.ant_glob('daemonNonGO.cpp')):
	    bld(target='daemonNonGO',
    	    features=['cxx', 'cxxprogram'],
        	source='daemonNonGO.cpp',
        	use='NDN_CXX')