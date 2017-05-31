#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

from waflib import Logs,Configure,Utils
def options(opt):
	opt.add_option('--debug','--with-debug',action='store_true',default=False,dest='debug',help='''Compile in debugging mode without optimizations (-O0 or -Og)''')
def configure(conf):
	cxx=conf.env['CXX_NAME']
	if cxx=='gcc':
		flags=GccFlags()
	elif cxx=='clang':
		flags=ClangFlags()
	else:
		flags=CompilerFlags()
		Logs.warn('The code has not yet been tested with %s compiler'%cxx)
	areCustomCxxflagsPresent=(len(conf.env.CXXFLAGS)>0)
	generalFlags=flags.getGeneralFlags(conf)
	conf.add_supported_cxxflags(generalFlags['CXXFLAGS'])
	conf.add_supported_linkflags(generalFlags['LINKFLAGS'])
	conf.env.DEFINES+=generalFlags['DEFINES']
	if conf.options.debug:
		extraFlags=flags.getDebugFlags(conf)
		if areCustomCxxflagsPresent:
			missingFlags=[x for x in extraFlags['CXXFLAGS']if x not in conf.env.CXXFLAGS]
			if len(missingFlags)>0:
				Logs.warn("Selected debug mode, but CXXFLAGS is set to a custom value '%s'"%" ".join(conf.env.CXXFLAGS))
				Logs.warn("Default flags '%s' are not activated"%" ".join(missingFlags))
	else:
		extraFlags=flags.getOptimizedFlags(conf)
	if not areCustomCxxflagsPresent:
		conf.add_supported_cxxflags(extraFlags['CXXFLAGS'])
		conf.add_supported_linkflags(extraFlags['LINKFLAGS'])
	conf.env.DEFINES+=extraFlags['DEFINES']
@Configure.conf
def add_supported_cxxflags(self,cxxflags):
	if len(cxxflags)==0:
		return
	self.start_msg('Checking supported CXXFLAGS')
	supportedFlags=[]
	for flag in cxxflags:
		if self.check_cxx(cxxflags=['-Werror',flag],mandatory=False):
			supportedFlags+=[flag]
	self.end_msg(' '.join(supportedFlags))
	self.env.prepend_value('CXXFLAGS',supportedFlags)
@Configure.conf
def add_supported_linkflags(self,linkflags):
	if len(linkflags)==0:
		return
	self.start_msg('Checking supported LINKFLAGS')
	supportedFlags=[]
	for flag in linkflags:
		if self.check_cxx(linkflags=['-Werror',flag],mandatory=False):
			supportedFlags+=[flag]
	self.end_msg(' '.join(supportedFlags))
	self.env.prepend_value('LINKFLAGS',supportedFlags)
class CompilerFlags(object):
	def getGeneralFlags(self,conf):
		return{'CXXFLAGS':[],'LINKFLAGS':[],'DEFINES':[]}
	def getDebugFlags(self,conf):
		return{'CXXFLAGS':[],'LINKFLAGS':[],'DEFINES':['_DEBUG']}
	def getOptimizedFlags(self,conf):
		return{'CXXFLAGS':[],'LINKFLAGS':[],'DEFINES':['NDEBUG']}
class GccBasicFlags(CompilerFlags):
	def getDebugFlags(self,conf):
		flags=super(GccBasicFlags,self).getDebugFlags(conf)
		flags['CXXFLAGS']+=['-O0','-g3','-pedantic','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-error=maybe-uninitialized',]
		return flags
	def getOptimizedFlags(self,conf):
		flags=super(GccBasicFlags,self).getOptimizedFlags(conf)
		flags['CXXFLAGS']+=['-O2','-g','-pedantic','-Wall','-Wextra','-Wno-unused-parameter',]
		return flags
class GccFlags(GccBasicFlags):
	def getGeneralFlags(self,conf):
		flags=super(GccFlags,self).getGeneralFlags(conf)
		version=tuple(int(i)for i in conf.env['CC_VERSION'])
		if version<(4,8,2):
			conf.fatal('The version of gcc you are using (%s) is too old.\n'%'.'.join(conf.env['CC_VERSION'])+'The minimum supported gcc version is 4.8.2.')
		else:
			flags['CXXFLAGS']+=['-std=c++11']
		return flags
	def getDebugFlags(self,conf):
		flags=super(GccFlags,self).getDebugFlags(conf)
		version=tuple(int(i)for i in conf.env['CC_VERSION'])
		if version<(5,1,0):
			flags['CXXFLAGS']+=['-Wno-missing-field-initializers']
		flags['CXXFLAGS']+=['-Og','-fdiagnostics-color',]
		return flags
	def getOptimizedFlags(self,conf):
		flags=super(GccFlags,self).getOptimizedFlags(conf)
		version=tuple(int(i)for i in conf.env['CC_VERSION'])
		if version<(5,1,0):
			flags['CXXFLAGS']+=['-Wno-missing-field-initializers']
		flags['CXXFLAGS']+=['-fdiagnostics-color']
		return flags
class ClangFlags(GccBasicFlags):
	def getGeneralFlags(self,conf):
		flags=super(ClangFlags,self).getGeneralFlags(conf)
		flags['CXXFLAGS']+=['-std=c++11']
		if Utils.unversioned_sys_platform()=='darwin':
			flags['CXXFLAGS']+=['-stdlib=libc++']
			flags['LINKFLAGS']+=['-stdlib=libc++']
		return flags
	def getDebugFlags(self,conf):
		flags=super(ClangFlags,self).getDebugFlags(conf)
		flags['CXXFLAGS']+=['-fcolor-diagnostics','-Wno-unused-local-typedef','-Wno-error=unneeded-internal-declaration','-Wno-error=deprecated-register','-Wno-error=keyword-macro','-Wno-error=infinite-recursion',]
		return flags
	def getOptimizedFlags(self,conf):
		flags=super(ClangFlags,self).getOptimizedFlags(conf)
		flags['CXXFLAGS']+=['-fcolor-diagnostics','-Wno-unused-local-typedef',]
		return flags
