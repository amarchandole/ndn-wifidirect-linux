AR = ['/opt/local/bin/ar']
ARCH_ST = ['-arch']
ARFLAGS = ['rcs']
BINDIR = '/usr/local/bin'
CC_VERSION = ('8', '1', '0')
COMPILER_CXX = 'clang++'
CPPPATH_ST = '-I%s'
CXX = ['/usr/bin/clang++']
CXXFLAGS = ['-O2', '-g', '-pedantic', '-Wall', '-Wextra', '-Wno-unused-parameter', '-fcolor-diagnostics', '-Wno-unused-local-typedef', '-std=c++11', '-stdlib=libc++']
CXXFLAGS_MACBUNDLE = ['-fPIC']
CXXFLAGS_cxxshlib = ['-fPIC']
CXXLNK_SRC_F = []
CXXLNK_TGT_F = ['-o']
CXX_NAME = 'clang'
CXX_SRC_F = []
CXX_TGT_F = ['-c', '-o']
DEFINES = ['NDEBUG', 'HAVE_NDN_CXX=1']
DEFINES_ST = '-D%s'
DEFINE_COMMENTS = {'HAVE_NDN_CXX': ''}
DEST_BINFMT = 'mac-o'
DEST_CPU = 'x86_64'
DEST_OS = 'darwin'
FRAMEWORKPATH_ST = '-F%s'
FRAMEWORK_NDN_CXX = ['CoreFoundation', 'Security']
FRAMEWORK_ST = ['-framework']
HAVE_NDN_CXX = 1
INCLUDES_NDN_CXX = ['/usr/local/include', '/usr/local/opt/boost@1.59/include', '/opt/local/include']
LIBDIR = '/usr/local/lib'
LIBPATH_NDN_CXX = ['/usr/local/lib', '/usr/local/opt/boost@1.59/lib', '/opt/local/lib']
LIBPATH_ST = '-L%s'
LIB_NDN_CXX = ['ndn-cxx', 'boost_system-mt', 'boost_filesystem-mt', 'boost_date_time-mt', 'boost_iostreams-mt', 'boost_regex-mt', 'boost_program_options-mt', 'boost_chrono-mt', 'boost_thread-mt', 'boost_log-mt', 'boost_log_setup-mt', 'cryptopp', 'ssl', 'crypto', 'sqlite3', 'pthread']
LIB_ST = '-l%s'
LINKFLAGS = ['-stdlib=libc++']
LINKFLAGS_MACBUNDLE = ['-bundle', '-undefined', 'dynamic_lookup']
LINKFLAGS_cxxshlib = ['-dynamiclib']
LINKFLAGS_cxxstlib = []
LINK_CXX = ['/usr/bin/clang++']
PKGCONFIG = ['/opt/local/bin/pkg-config']
PREFIX = '/usr/local'
RPATH_ST = '-Wl,-rpath,%s'
SHLIB_MARKER = []
SONAME_ST = []
STLIBPATH_ST = '-L%s'
STLIB_MARKER = []
STLIB_ST = '-l%s'
cxxprogram_PATTERN = '%s'
cxxshlib_PATTERN = 'lib%s.dylib'
cxxstlib_PATTERN = 'lib%s.a'
define_key = ['HAVE_NDN_CXX']
macbundle_PATTERN = '%s.bundle'