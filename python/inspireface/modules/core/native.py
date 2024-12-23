__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403
import platform
from pathlib import Path
import subprocess

def get_lib_path():
    """
    Get the appropriate library path based on the current platform and architecture.
    
    Returns:
        str: The path to the platform-specific library
        
    Raises:
        RuntimeError: If the platform/architecture is unsupported or library not found
    """
    package_dir = Path(__file__).parent
    
    # Get basic platform information
    system = platform.system().lower()
    machine = platform.machine().lower()
    
    # Initialize variables
    platform_dir = None
    lib_name = None
    arch = None
    
    if system == 'windows':
        platform_dir = 'windows'
        lib_name = 'libInspireFace.dll'
        # Windows architecture detection
        arch = 'x64' if machine == 'amd64' or machine == 'x86_64' else 'arm64'
        
    elif system == 'linux':
        platform_dir = 'linux'
        lib_name = 'libInspireFace.so'
        # Linux architecture detection
        if machine == 'x86_64':
            arch = 'x64'
        elif machine in ['aarch64', 'arm64']:
            arch = 'arm64'
        elif machine.startswith('arm'):
            arch = 'arm64'  # Might need more specific ARM version distinction
            
    elif system == 'darwin':  # macOS
        platform_dir = 'darwin'
        lib_name = 'libInspireFace.dylib'
        
        # macOS architecture detection
        if machine == 'x86_64':
            # Check if running under Rosetta 2
            try:
                # Use sysctl to detect Rosetta 2
                is_rosetta = bool(int(subprocess.check_output(
                    ['sysctl', '-n', 'sysctl.proc_translated']).decode().strip()))
                # If running under Rosetta, it's actually an ARM machine
                if is_rosetta:
                    arch = 'arm64'
                else:
                    arch = 'x64'
            except:
                # If detection fails, assume native x64
                arch = 'x64'
        elif machine == 'arm64':
            arch = 'arm64'
            
    # Validate that all necessary parameters were set
    if not all([platform_dir, lib_name, arch]):
        raise RuntimeError(
            f"Unsupported platform: system={system}, machine={machine}")
    
    # Construct the full library path
    lib_path = package_dir / 'libs' / platform_dir / arch / lib_name
    
    # Verify that the library file exists
    if not lib_path.exists():
        raise RuntimeError(
            f"Library not found at {lib_path}. "
            f"System: {system}, Architecture: {arch}")
    
    return str(lib_path)

try:    
    _LIBRARY_FILENAME = get_lib_path()
except Exception as e:
    print(e)

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types



class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, ctypes.Union):

    _fields_ = [("raw", ctypes.POINTER(ctypes.c_char)), ("data", ctypes.c_char_p)]

    def __init__(self, obj=b""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(ctypes.POINTER(ctypes.c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, ctypes.c_char_p):
            return obj

        # Convert from POINTER(ctypes.c_char)
        elif isinstance(obj, ctypes.POINTER(ctypes.c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(ctypes.cast(obj, ctypes.POINTER(ctypes.c_char)))

        # Convert from ctypes.c_char array
        elif isinstance(obj, ctypes.c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception as err:  # pylint: disable=broad-except
                print(err)

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "../../..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)


del loaderclass

# End loader

add_library_search_dirs([])

# Begin libraries
_libs[_LIBRARY_FILENAME] = load_library(_LIBRARY_FILENAME)

# 1 libraries
# End libraries

# No modules

uint8_t = c_ubyte# /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/_types/_uint8_t.h: 31

HPVoid = POINTER(None)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 10

HFImageStream = POINTER(None)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 11

HFSession = POINTER(None)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 12

HFImageBitmap = POINTER(None)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 13

HFloat = c_float# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 15

HPFloat = POINTER(c_float)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 16

HInt32 = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 19

HOption = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 20

HPInt32 = POINTER(c_int)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 21

HFaceId = c_int64# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 22

HPFaceId = POINTER(c_int64)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 23

HResult = c_long# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 24

HString = String# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 25

HPath = String# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 26

HChar = c_char# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 28

HPBuffer = String# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 29

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 39
class struct_HFaceRect(Structure):
    pass

struct_HFaceRect.__slots__ = [
    'x',
    'y',
    'width',
    'height',
]
struct_HFaceRect._fields_ = [
    ('x', HInt32),
    ('y', HInt32),
    ('width', HInt32),
    ('height', HInt32),
]

HFaceRect = struct_HFaceRect# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 39

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 44
class struct_HPoint2f(Structure):
    pass

struct_HPoint2f.__slots__ = [
    'x',
    'y',
]
struct_HPoint2f._fields_ = [
    ('x', HFloat),
    ('y', HFloat),
]

HPoint2f = struct_HPoint2f# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 44

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 49
class struct_HPoint2i(Structure):
    pass

struct_HPoint2i.__slots__ = [
    'x',
    'y',
]
struct_HPoint2i._fields_ = [
    ('x', HInt32),
    ('y', HInt32),
]

HPoint2i = struct_HPoint2i# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 49

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 55
class struct_HColor(Structure):
    pass

struct_HColor.__slots__ = [
    'r',
    'g',
    'b',
]
struct_HColor._fields_ = [
    ('r', HFloat),
    ('g', HFloat),
    ('b', HFloat),
]

HColor = struct_HColor# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/intypedef.h: 55

enum_HFImageFormat = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_RGB = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_BGR = 1# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_RGBA = 2# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_BGRA = 3# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_YUV_NV12 = 4# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HF_STREAM_YUV_NV21 = 5# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

HFImageFormat = enum_HFImageFormat# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 48

enum_HFRotation = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

HF_CAMERA_ROTATION_0 = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

HF_CAMERA_ROTATION_90 = 1# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

HF_CAMERA_ROTATION_180 = 2# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

HF_CAMERA_ROTATION_270 = 3# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

HFRotation = enum_HFRotation# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 59

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 71
class struct_HFImageData(Structure):
    pass

struct_HFImageData.__slots__ = [
    'data',
    'width',
    'height',
    'format',
    'rotation',
]
struct_HFImageData._fields_ = [
    ('data', POINTER(uint8_t)),
    ('width', HInt32),
    ('height', HInt32),
    ('format', HFImageFormat),
    ('rotation', HFRotation),
]

HFImageData = struct_HFImageData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 71

PHFImageData = POINTER(struct_HFImageData)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 71

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 82
if _libs[_LIBRARY_FILENAME].has("HFCreateImageStream", "cdecl"):
    HFCreateImageStream = _libs[_LIBRARY_FILENAME].get("HFCreateImageStream", "cdecl")
    HFCreateImageStream.argtypes = [PHFImageData, POINTER(HFImageStream)]
    HFCreateImageStream.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 92
if _libs[_LIBRARY_FILENAME].has("HFReleaseImageStream", "cdecl"):
    HFReleaseImageStream = _libs[_LIBRARY_FILENAME].get("HFReleaseImageStream", "cdecl")
    HFReleaseImageStream.argtypes = [HFImageStream]
    HFReleaseImageStream.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 102
class struct_HFImageBitmapData(Structure):
    pass

struct_HFImageBitmapData.__slots__ = [
    'data',
    'width',
    'height',
    'channels',
]
struct_HFImageBitmapData._fields_ = [
    ('data', POINTER(uint8_t)),
    ('width', HInt32),
    ('height', HInt32),
    ('channels', HInt32),
]

HFImageBitmapData = struct_HFImageBitmapData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 102

PHFImageBitmapData = POINTER(struct_HFImageBitmapData)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 102

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 111
if _libs[_LIBRARY_FILENAME].has("HFCreateImageBitmap", "cdecl"):
    HFCreateImageBitmap = _libs[_LIBRARY_FILENAME].get("HFCreateImageBitmap", "cdecl")
    HFCreateImageBitmap.argtypes = [PHFImageBitmapData, POINTER(HFImageBitmap)]
    HFCreateImageBitmap.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 121
if _libs[_LIBRARY_FILENAME].has("HFCreateImageBitmapFromFilePath", "cdecl"):
    HFCreateImageBitmapFromFilePath = _libs[_LIBRARY_FILENAME].get("HFCreateImageBitmapFromFilePath", "cdecl")
    HFCreateImageBitmapFromFilePath.argtypes = [HPath, HInt32, POINTER(HFImageBitmap)]
    HFCreateImageBitmapFromFilePath.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 130
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapCopy", "cdecl"):
    HFImageBitmapCopy = _libs[_LIBRARY_FILENAME].get("HFImageBitmapCopy", "cdecl")
    HFImageBitmapCopy.argtypes = [HFImageBitmap, POINTER(HFImageBitmap)]
    HFImageBitmapCopy.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 138
if _libs[_LIBRARY_FILENAME].has("HFReleaseImageBitmap", "cdecl"):
    HFReleaseImageBitmap = _libs[_LIBRARY_FILENAME].get("HFReleaseImageBitmap", "cdecl")
    HFReleaseImageBitmap.argtypes = [HFImageBitmap]
    HFReleaseImageBitmap.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 148
if _libs[_LIBRARY_FILENAME].has("HFCreateImageStreamFromImageBitmap", "cdecl"):
    HFCreateImageStreamFromImageBitmap = _libs[_LIBRARY_FILENAME].get("HFCreateImageStreamFromImageBitmap", "cdecl")
    HFCreateImageStreamFromImageBitmap.argtypes = [HFImageBitmap, HFRotation, POINTER(HFImageStream)]
    HFCreateImageStreamFromImageBitmap.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 157
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapWriteToFile", "cdecl"):
    HFImageBitmapWriteToFile = _libs[_LIBRARY_FILENAME].get("HFImageBitmapWriteToFile", "cdecl")
    HFImageBitmapWriteToFile.argtypes = [HFImageBitmap, HPath]
    HFImageBitmapWriteToFile.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 168
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapDrawRect", "cdecl"):
    HFImageBitmapDrawRect = _libs[_LIBRARY_FILENAME].get("HFImageBitmapDrawRect", "cdecl")
    HFImageBitmapDrawRect.argtypes = [HFImageBitmap, HFaceRect, HColor, HInt32]
    HFImageBitmapDrawRect.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 180
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapDrawCircleF", "cdecl"):
    HFImageBitmapDrawCircleF = _libs[_LIBRARY_FILENAME].get("HFImageBitmapDrawCircleF", "cdecl")
    HFImageBitmapDrawCircleF.argtypes = [HFImageBitmap, HPoint2f, HInt32, HColor, HInt32]
    HFImageBitmapDrawCircleF.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 181
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapDrawCircle", "cdecl"):
    HFImageBitmapDrawCircle = _libs[_LIBRARY_FILENAME].get("HFImageBitmapDrawCircle", "cdecl")
    HFImageBitmapDrawCircle.argtypes = [HFImageBitmap, HPoint2i, HInt32, HColor, HInt32]
    HFImageBitmapDrawCircle.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 190
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapGetData", "cdecl"):
    HFImageBitmapGetData = _libs[_LIBRARY_FILENAME].get("HFImageBitmapGetData", "cdecl")
    HFImageBitmapGetData.argtypes = [HFImageBitmap, PHFImageBitmapData]
    HFImageBitmapGetData.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 200
if _libs[_LIBRARY_FILENAME].has("HFImageBitmapShow", "cdecl"):
    HFImageBitmapShow = _libs[_LIBRARY_FILENAME].get("HFImageBitmapShow", "cdecl")
    HFImageBitmapShow.argtypes = [HFImageBitmap, HString, HInt32]
    HFImageBitmapShow.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 214
if _libs[_LIBRARY_FILENAME].has("HFLaunchInspireFace", "cdecl"):
    HFLaunchInspireFace = _libs[_LIBRARY_FILENAME].get("HFLaunchInspireFace", "cdecl")
    HFLaunchInspireFace.argtypes = [HPath]
    HFLaunchInspireFace.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 222
if _libs[_LIBRARY_FILENAME].has("HFReloadInspireFace", "cdecl"):
    HFReloadInspireFace = _libs[_LIBRARY_FILENAME].get("HFReloadInspireFace", "cdecl")
    HFReloadInspireFace.argtypes = [HPath]
    HFReloadInspireFace.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 230
if _libs[_LIBRARY_FILENAME].has("HFTerminateInspireFace", "cdecl"):
    HFTerminateInspireFace = _libs[_LIBRARY_FILENAME].get("HFTerminateInspireFace", "cdecl")
    HFTerminateInspireFace.argtypes = []
    HFTerminateInspireFace.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 238
if _libs[_LIBRARY_FILENAME].has("HFQueryInspireFaceLaunchStatus", "cdecl"):
    HFQueryInspireFaceLaunchStatus = _libs[_LIBRARY_FILENAME].get("HFQueryInspireFaceLaunchStatus", "cdecl")
    HFQueryInspireFaceLaunchStatus.argtypes = [POINTER(HInt32)]
    HFQueryInspireFaceLaunchStatus.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 259
class struct_HFSessionCustomParameter(Structure):
    pass

struct_HFSessionCustomParameter.__slots__ = [
    'enable_recognition',
    'enable_liveness',
    'enable_ir_liveness',
    'enable_mask_detect',
    'enable_face_quality',
    'enable_face_attribute',
    'enable_interaction_liveness',
]
struct_HFSessionCustomParameter._fields_ = [
    ('enable_recognition', HInt32),
    ('enable_liveness', HInt32),
    ('enable_ir_liveness', HInt32),
    ('enable_mask_detect', HInt32),
    ('enable_face_quality', HInt32),
    ('enable_face_attribute', HInt32),
    ('enable_interaction_liveness', HInt32),
]

HFSessionCustomParameter = struct_HFSessionCustomParameter# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 259

PHFSessionCustomParameter = POINTER(struct_HFSessionCustomParameter)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 259

enum_HFDetectMode = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 272

HF_DETECT_MODE_ALWAYS_DETECT = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 272

HF_DETECT_MODE_LIGHT_TRACK = (HF_DETECT_MODE_ALWAYS_DETECT + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 272

HF_DETECT_MODE_TRACK_BY_DETECTION = (HF_DETECT_MODE_LIGHT_TRACK + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 272

HFDetectMode = enum_HFDetectMode# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 272

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 288
if _libs[_LIBRARY_FILENAME].has("HFCreateInspireFaceSession", "cdecl"):
    HFCreateInspireFaceSession = _libs[_LIBRARY_FILENAME].get("HFCreateInspireFaceSession", "cdecl")
    HFCreateInspireFaceSession.argtypes = [HFSessionCustomParameter, HFDetectMode, HInt32, HInt32, HInt32, POINTER(HFSession)]
    HFCreateInspireFaceSession.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 305
if _libs[_LIBRARY_FILENAME].has("HFCreateInspireFaceSessionOptional", "cdecl"):
    HFCreateInspireFaceSessionOptional = _libs[_LIBRARY_FILENAME].get("HFCreateInspireFaceSessionOptional", "cdecl")
    HFCreateInspireFaceSessionOptional.argtypes = [HOption, HFDetectMode, HInt32, HInt32, HInt32, POINTER(HFSession)]
    HFCreateInspireFaceSessionOptional.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 314
if _libs[_LIBRARY_FILENAME].has("HFReleaseInspireFaceSession", "cdecl"):
    HFReleaseInspireFaceSession = _libs[_LIBRARY_FILENAME].get("HFReleaseInspireFaceSession", "cdecl")
    HFReleaseInspireFaceSession.argtypes = [HFSession]
    HFReleaseInspireFaceSession.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 324
class struct_HFFaceBasicToken(Structure):
    pass

struct_HFFaceBasicToken.__slots__ = [
    'size',
    'data',
]
struct_HFFaceBasicToken._fields_ = [
    ('size', HInt32),
    ('data', HPVoid),
]

HFFaceBasicToken = struct_HFFaceBasicToken# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 324

PHFFaceBasicToken = POINTER(struct_HFFaceBasicToken)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 324

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 335
class struct_HFFaceEulerAngle(Structure):
    pass

struct_HFFaceEulerAngle.__slots__ = [
    'roll',
    'yaw',
    'pitch',
]
struct_HFFaceEulerAngle._fields_ = [
    ('roll', POINTER(HFloat)),
    ('yaw', POINTER(HFloat)),
    ('pitch', POINTER(HFloat)),
]

HFFaceEulerAngle = struct_HFFaceEulerAngle# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 335

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 350
class struct_HFMultipleFaceData(Structure):
    pass

struct_HFMultipleFaceData.__slots__ = [
    'detectedNum',
    'rects',
    'trackIds',
    'detConfidence',
    'angles',
    'tokens',
]
struct_HFMultipleFaceData._fields_ = [
    ('detectedNum', HInt32),
    ('rects', POINTER(HFaceRect)),
    ('trackIds', POINTER(HInt32)),
    ('detConfidence', POINTER(HFloat)),
    ('angles', HFFaceEulerAngle),
    ('tokens', PHFFaceBasicToken),
]

HFMultipleFaceData = struct_HFMultipleFaceData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 350

PHFMultipleFaceData = POINTER(struct_HFMultipleFaceData)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 350

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 360
if _libs[_LIBRARY_FILENAME].has("HFSessionSetTrackPreviewSize", "cdecl"):
    HFSessionSetTrackPreviewSize = _libs[_LIBRARY_FILENAME].get("HFSessionSetTrackPreviewSize", "cdecl")
    HFSessionSetTrackPreviewSize.argtypes = [HFSession, HInt32]
    HFSessionSetTrackPreviewSize.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 370
if _libs[_LIBRARY_FILENAME].has("HFSessionSetFilterMinimumFacePixelSize", "cdecl"):
    HFSessionSetFilterMinimumFacePixelSize = _libs[_LIBRARY_FILENAME].get("HFSessionSetFilterMinimumFacePixelSize", "cdecl")
    HFSessionSetFilterMinimumFacePixelSize.argtypes = [HFSession, HInt32]
    HFSessionSetFilterMinimumFacePixelSize.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 379
if _libs[_LIBRARY_FILENAME].has("HFSessionSetFaceDetectThreshold", "cdecl"):
    HFSessionSetFaceDetectThreshold = _libs[_LIBRARY_FILENAME].get("HFSessionSetFaceDetectThreshold", "cdecl")
    HFSessionSetFaceDetectThreshold.argtypes = [HFSession, HFloat]
    HFSessionSetFaceDetectThreshold.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 389
if _libs[_LIBRARY_FILENAME].has("HFExecuteFaceTrack", "cdecl"):
    HFExecuteFaceTrack = _libs[_LIBRARY_FILENAME].get("HFExecuteFaceTrack", "cdecl")
    HFExecuteFaceTrack.argtypes = [HFSession, HFImageStream, PHFMultipleFaceData]
    HFExecuteFaceTrack.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 406
if _libs[_LIBRARY_FILENAME].has("HFCopyFaceBasicToken", "cdecl"):
    HFCopyFaceBasicToken = _libs[_LIBRARY_FILENAME].get("HFCopyFaceBasicToken", "cdecl")
    HFCopyFaceBasicToken.argtypes = [HFFaceBasicToken, HPBuffer, HInt32]
    HFCopyFaceBasicToken.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 420
if _libs[_LIBRARY_FILENAME].has("HFGetFaceBasicTokenSize", "cdecl"):
    HFGetFaceBasicTokenSize = _libs[_LIBRARY_FILENAME].get("HFGetFaceBasicTokenSize", "cdecl")
    HFGetFaceBasicTokenSize.argtypes = [HPInt32]
    HFGetFaceBasicTokenSize.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 427
if _libs[_LIBRARY_FILENAME].has("HFGetNumOfFaceDenseLandmark", "cdecl"):
    HFGetNumOfFaceDenseLandmark = _libs[_LIBRARY_FILENAME].get("HFGetNumOfFaceDenseLandmark", "cdecl")
    HFGetNumOfFaceDenseLandmark.argtypes = [HPInt32]
    HFGetNumOfFaceDenseLandmark.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 437
if _libs[_LIBRARY_FILENAME].has("HFGetFaceDenseLandmarkFromFaceToken", "cdecl"):
    HFGetFaceDenseLandmarkFromFaceToken = _libs[_LIBRARY_FILENAME].get("HFGetFaceDenseLandmarkFromFaceToken", "cdecl")
    HFGetFaceDenseLandmarkFromFaceToken.argtypes = [HFFaceBasicToken, POINTER(HPoint2f), HInt32]
    HFGetFaceDenseLandmarkFromFaceToken.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 451
class struct_HFFaceFeature(Structure):
    pass

struct_HFFaceFeature.__slots__ = [
    'size',
    'data',
]
struct_HFFaceFeature._fields_ = [
    ('size', HInt32),
    ('data', HPFloat),
]

HFFaceFeature = struct_HFFaceFeature# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 451

PHFFaceFeature = POINTER(struct_HFFaceFeature)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 451

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 462
if _libs[_LIBRARY_FILENAME].has("HFFaceFeatureExtract", "cdecl"):
    HFFaceFeatureExtract = _libs[_LIBRARY_FILENAME].get("HFFaceFeatureExtract", "cdecl")
    HFFaceFeatureExtract.argtypes = [HFSession, HFImageStream, HFFaceBasicToken, PHFFaceFeature]
    HFFaceFeatureExtract.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 474
if _libs[_LIBRARY_FILENAME].has("HFFaceFeatureExtractCpy", "cdecl"):
    HFFaceFeatureExtractCpy = _libs[_LIBRARY_FILENAME].get("HFFaceFeatureExtractCpy", "cdecl")
    HFFaceFeatureExtractCpy.argtypes = [HFSession, HFImageStream, HFFaceBasicToken, HPFloat]
    HFFaceFeatureExtractCpy.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 484
if _libs[_LIBRARY_FILENAME].has("HFFaceGetFaceAlignmentImage", "cdecl"):
    HFFaceGetFaceAlignmentImage = _libs[_LIBRARY_FILENAME].get("HFFaceGetFaceAlignmentImage", "cdecl")
    HFFaceGetFaceAlignmentImage.argtypes = [HFSession, HFImageStream, HFFaceBasicToken, POINTER(HFImageBitmap)]
    HFFaceGetFaceAlignmentImage.restype = HResult

enum_HFSearchMode = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 498

HF_SEARCH_MODE_EAGER = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 498

HF_SEARCH_MODE_EXHAUSTIVE = (HF_SEARCH_MODE_EAGER + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 498

HFSearchMode = enum_HFSearchMode# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 498

enum_HFPKMode = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 506

HF_PK_AUTO_INCREMENT = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 506

HF_PK_MANUAL_INPUT = (HF_PK_AUTO_INCREMENT + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 506

HFPKMode = enum_HFPKMode# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 506

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 519
class struct_HFFeatureHubConfiguration(Structure):
    pass

struct_HFFeatureHubConfiguration.__slots__ = [
    'primaryKeyMode',
    'enablePersistence',
    'persistenceDbPath',
    'searchThreshold',
    'searchMode',
]
struct_HFFeatureHubConfiguration._fields_ = [
    ('primaryKeyMode', HFPKMode),
    ('enablePersistence', HInt32),
    ('persistenceDbPath', HString),
    ('searchThreshold', c_float),
    ('searchMode', HFSearchMode),
]

HFFeatureHubConfiguration = struct_HFFeatureHubConfiguration# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 519

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 531
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubDataEnable", "cdecl"):
    HFFeatureHubDataEnable = _libs[_LIBRARY_FILENAME].get("HFFeatureHubDataEnable", "cdecl")
    HFFeatureHubDataEnable.argtypes = [HFFeatureHubConfiguration]
    HFFeatureHubDataEnable.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 537
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubDataDisable", "cdecl"):
    HFFeatureHubDataDisable = _libs[_LIBRARY_FILENAME].get("HFFeatureHubDataDisable", "cdecl")
    HFFeatureHubDataDisable.argtypes = []
    HFFeatureHubDataDisable.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 548
class struct_HFFaceFeatureIdentity(Structure):
    pass

struct_HFFaceFeatureIdentity.__slots__ = [
    'id',
    'feature',
]
struct_HFFaceFeatureIdentity._fields_ = [
    ('id', HFaceId),
    ('feature', PHFFaceFeature),
]

HFFaceFeatureIdentity = struct_HFFaceFeatureIdentity# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 548

PHFFaceFeatureIdentity = POINTER(struct_HFFaceFeatureIdentity)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 548

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 557
class struct_HFSearchTopKResults(Structure):
    pass

struct_HFSearchTopKResults.__slots__ = [
    'size',
    'confidence',
    'ids',
]
struct_HFSearchTopKResults._fields_ = [
    ('size', HInt32),
    ('confidence', HPFloat),
    ('ids', HPFaceId),
]

HFSearchTopKResults = struct_HFSearchTopKResults# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 557

PHFSearchTopKResults = POINTER(struct_HFSearchTopKResults)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 557

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 569
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubFaceSearchThresholdSetting", "cdecl"):
    HFFeatureHubFaceSearchThresholdSetting = _libs[_LIBRARY_FILENAME].get("HFFeatureHubFaceSearchThresholdSetting", "cdecl")
    HFFeatureHubFaceSearchThresholdSetting.argtypes = [c_float]
    HFFeatureHubFaceSearchThresholdSetting.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 580
if _libs[_LIBRARY_FILENAME].has("HFFaceComparison", "cdecl"):
    HFFaceComparison = _libs[_LIBRARY_FILENAME].get("HFFaceComparison", "cdecl")
    HFFaceComparison.argtypes = [HFFaceFeature, HFFaceFeature, HPFloat]
    HFFaceComparison.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 588
if _libs[_LIBRARY_FILENAME].has("HFGetFeatureLength", "cdecl"):
    HFGetFeatureLength = _libs[_LIBRARY_FILENAME].get("HFGetFeatureLength", "cdecl")
    HFGetFeatureLength.argtypes = [HPInt32]
    HFGetFeatureLength.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 596
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubInsertFeature", "cdecl"):
    HFFeatureHubInsertFeature = _libs[_LIBRARY_FILENAME].get("HFFeatureHubInsertFeature", "cdecl")
    HFFeatureHubInsertFeature.argtypes = [HFFaceFeatureIdentity, HPFaceId]
    HFFeatureHubInsertFeature.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 607
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubFaceSearch", "cdecl"):
    HFFeatureHubFaceSearch = _libs[_LIBRARY_FILENAME].get("HFFeatureHubFaceSearch", "cdecl")
    HFFeatureHubFaceSearch.argtypes = [HFFaceFeature, HPFloat, PHFFaceFeatureIdentity]
    HFFeatureHubFaceSearch.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 617
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubFaceSearchTopK", "cdecl"):
    HFFeatureHubFaceSearchTopK = _libs[_LIBRARY_FILENAME].get("HFFeatureHubFaceSearchTopK", "cdecl")
    HFFeatureHubFaceSearchTopK.argtypes = [HFFaceFeature, HInt32, PHFSearchTopKResults]
    HFFeatureHubFaceSearchTopK.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 625
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubFaceRemove", "cdecl"):
    HFFeatureHubFaceRemove = _libs[_LIBRARY_FILENAME].get("HFFeatureHubFaceRemove", "cdecl")
    HFFeatureHubFaceRemove.argtypes = [HFaceId]
    HFFeatureHubFaceRemove.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 633
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubFaceUpdate", "cdecl"):
    HFFeatureHubFaceUpdate = _libs[_LIBRARY_FILENAME].get("HFFeatureHubFaceUpdate", "cdecl")
    HFFeatureHubFaceUpdate.argtypes = [HFFaceFeatureIdentity]
    HFFeatureHubFaceUpdate.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 642
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubGetFaceIdentity", "cdecl"):
    HFFeatureHubGetFaceIdentity = _libs[_LIBRARY_FILENAME].get("HFFeatureHubGetFaceIdentity", "cdecl")
    HFFeatureHubGetFaceIdentity.argtypes = [HFaceId, PHFFaceFeatureIdentity]
    HFFeatureHubGetFaceIdentity.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 650
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubGetFaceCount", "cdecl"):
    HFFeatureHubGetFaceCount = _libs[_LIBRARY_FILENAME].get("HFFeatureHubGetFaceCount", "cdecl")
    HFFeatureHubGetFaceCount.argtypes = [POINTER(HInt32)]
    HFFeatureHubGetFaceCount.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 657
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubViewDBTable", "cdecl"):
    HFFeatureHubViewDBTable = _libs[_LIBRARY_FILENAME].get("HFFeatureHubViewDBTable", "cdecl")
    HFFeatureHubViewDBTable.argtypes = []
    HFFeatureHubViewDBTable.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 665
class struct_HFFeatureHubExistingIds(Structure):
    pass

struct_HFFeatureHubExistingIds.__slots__ = [
    'size',
    'ids',
]
struct_HFFeatureHubExistingIds._fields_ = [
    ('size', HInt32),
    ('ids', HPFaceId),
]

HFFeatureHubExistingIds = struct_HFFeatureHubExistingIds# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 665

PHFFeatureHubExistingIds = POINTER(struct_HFFeatureHubExistingIds)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 665

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 672
if _libs[_LIBRARY_FILENAME].has("HFFeatureHubGetExistingIds", "cdecl"):
    HFFeatureHubGetExistingIds = _libs[_LIBRARY_FILENAME].get("HFFeatureHubGetExistingIds", "cdecl")
    HFFeatureHubGetExistingIds.argtypes = [PHFFeatureHubExistingIds]
    HFFeatureHubGetExistingIds.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 690
if _libs[_LIBRARY_FILENAME].has("HFMultipleFacePipelineProcess", "cdecl"):
    HFMultipleFacePipelineProcess = _libs[_LIBRARY_FILENAME].get("HFMultipleFacePipelineProcess", "cdecl")
    HFMultipleFacePipelineProcess.argtypes = [HFSession, HFImageStream, PHFMultipleFaceData, HFSessionCustomParameter]
    HFMultipleFacePipelineProcess.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 705
if _libs[_LIBRARY_FILENAME].has("HFMultipleFacePipelineProcessOptional", "cdecl"):
    HFMultipleFacePipelineProcessOptional = _libs[_LIBRARY_FILENAME].get("HFMultipleFacePipelineProcessOptional", "cdecl")
    HFMultipleFacePipelineProcessOptional.argtypes = [HFSession, HFImageStream, PHFMultipleFaceData, HInt32]
    HFMultipleFacePipelineProcessOptional.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 717
class struct_HFRGBLivenessConfidence(Structure):
    pass

struct_HFRGBLivenessConfidence.__slots__ = [
    'num',
    'confidence',
]
struct_HFRGBLivenessConfidence._fields_ = [
    ('num', HInt32),
    ('confidence', HPFloat),
]

HFRGBLivenessConfidence = struct_HFRGBLivenessConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 717

PHFRGBLivenessConfidence = POINTER(struct_HFRGBLivenessConfidence)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 717

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 729
if _libs[_LIBRARY_FILENAME].has("HFGetRGBLivenessConfidence", "cdecl"):
    HFGetRGBLivenessConfidence = _libs[_LIBRARY_FILENAME].get("HFGetRGBLivenessConfidence", "cdecl")
    HFGetRGBLivenessConfidence.argtypes = [HFSession, PHFRGBLivenessConfidence]
    HFGetRGBLivenessConfidence.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 740
class struct_HFFaceMaskConfidence(Structure):
    pass

struct_HFFaceMaskConfidence.__slots__ = [
    'num',
    'confidence',
]
struct_HFFaceMaskConfidence._fields_ = [
    ('num', HInt32),
    ('confidence', HPFloat),
]

HFFaceMaskConfidence = struct_HFFaceMaskConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 740

PHFFaceMaskConfidence = POINTER(struct_HFFaceMaskConfidence)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 740

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 752
if _libs[_LIBRARY_FILENAME].has("HFGetFaceMaskConfidence", "cdecl"):
    HFGetFaceMaskConfidence = _libs[_LIBRARY_FILENAME].get("HFGetFaceMaskConfidence", "cdecl")
    HFGetFaceMaskConfidence.argtypes = [HFSession, PHFFaceMaskConfidence]
    HFGetFaceMaskConfidence.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 763
class struct_HFFaceQualityConfidence(Structure):
    pass

struct_HFFaceQualityConfidence.__slots__ = [
    'num',
    'confidence',
]
struct_HFFaceQualityConfidence._fields_ = [
    ('num', HInt32),
    ('confidence', HPFloat),
]

HFFaceQualityConfidence = struct_HFFaceQualityConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 763

PHFFaceQualityConfidence = POINTER(struct_HFFaceQualityConfidence)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 763

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 775
if _libs[_LIBRARY_FILENAME].has("HFGetFaceQualityConfidence", "cdecl"):
    HFGetFaceQualityConfidence = _libs[_LIBRARY_FILENAME].get("HFGetFaceQualityConfidence", "cdecl")
    HFGetFaceQualityConfidence.argtypes = [HFSession, PHFFaceQualityConfidence]
    HFGetFaceQualityConfidence.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 787
if _libs[_LIBRARY_FILENAME].has("HFFaceQualityDetect", "cdecl"):
    HFFaceQualityDetect = _libs[_LIBRARY_FILENAME].get("HFFaceQualityDetect", "cdecl")
    HFFaceQualityDetect.argtypes = [HFSession, HFFaceBasicToken, POINTER(HFloat)]
    HFFaceQualityDetect.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 798
class struct_HFFaceInteractionState(Structure):
    pass

struct_HFFaceInteractionState.__slots__ = [
    'num',
    'leftEyeStatusConfidence',
    'rightEyeStatusConfidence',
]
struct_HFFaceInteractionState._fields_ = [
    ('num', HInt32),
    ('leftEyeStatusConfidence', HPFloat),
    ('rightEyeStatusConfidence', HPFloat),
]

HFFaceInteractionState = struct_HFFaceInteractionState# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 798

PHFFaceInteractionState = POINTER(struct_HFFaceInteractionState)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 798

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 805
if _libs[_LIBRARY_FILENAME].has("HFGetFaceInteractionStateResult", "cdecl"):
    HFGetFaceInteractionStateResult = _libs[_LIBRARY_FILENAME].get("HFGetFaceInteractionStateResult", "cdecl")
    HFGetFaceInteractionStateResult.argtypes = [HFSession, PHFFaceInteractionState]
    HFGetFaceInteractionStateResult.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 817
class struct_HFFaceInteractionsActions(Structure):
    pass

struct_HFFaceInteractionsActions.__slots__ = [
    'num',
    'normal',
    'shake',
    'jawOpen',
    'headRaise',
    'blink',
]
struct_HFFaceInteractionsActions._fields_ = [
    ('num', HInt32),
    ('normal', HPInt32),
    ('shake', HPInt32),
    ('jawOpen', HPInt32),
    ('headRaise', HPInt32),
    ('blink', HPInt32),
]

HFFaceInteractionsActions = struct_HFFaceInteractionsActions# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 817

PHFFaceInteractionsActions = POINTER(struct_HFFaceInteractionsActions)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 817

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 825
if _libs[_LIBRARY_FILENAME].has("HFGetFaceInteractionActionsResult", "cdecl"):
    HFGetFaceInteractionActionsResult = _libs[_LIBRARY_FILENAME].get("HFGetFaceInteractionActionsResult", "cdecl")
    HFGetFaceInteractionActionsResult.argtypes = [HFSession, PHFFaceInteractionsActions]
    HFGetFaceInteractionActionsResult.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 852
class struct_HFFaceAttributeResult(Structure):
    pass

struct_HFFaceAttributeResult.__slots__ = [
    'num',
    'race',
    'gender',
    'ageBracket',
]
struct_HFFaceAttributeResult._fields_ = [
    ('num', HInt32),
    ('race', HPInt32),
    ('gender', HPInt32),
    ('ageBracket', HPInt32),
]

HFFaceAttributeResult = struct_HFFaceAttributeResult# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 852

PHFFaceAttributeResult = POINTER(struct_HFFaceAttributeResult)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 852

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 864
if _libs[_LIBRARY_FILENAME].has("HFGetFaceAttributeResult", "cdecl"):
    HFGetFaceAttributeResult = _libs[_LIBRARY_FILENAME].get("HFGetFaceAttributeResult", "cdecl")
    HFGetFaceAttributeResult.argtypes = [HFSession, PHFFaceAttributeResult]
    HFGetFaceAttributeResult.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 877
class struct_HFInspireFaceVersion(Structure):
    pass

struct_HFInspireFaceVersion.__slots__ = [
    'major',
    'minor',
    'patch',
]
struct_HFInspireFaceVersion._fields_ = [
    ('major', c_int),
    ('minor', c_int),
    ('patch', c_int),
]

HFInspireFaceVersion = struct_HFInspireFaceVersion# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 877

PHFInspireFaceVersion = POINTER(struct_HFInspireFaceVersion)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 877

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 887
if _libs[_LIBRARY_FILENAME].has("HFQueryInspireFaceVersion", "cdecl"):
    HFQueryInspireFaceVersion = _libs[_LIBRARY_FILENAME].get("HFQueryInspireFaceVersion", "cdecl")
    HFQueryInspireFaceVersion.argtypes = [PHFInspireFaceVersion]
    HFQueryInspireFaceVersion.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 895
class struct_HFInspireFaceExtendedInformation(Structure):
    pass

struct_HFInspireFaceExtendedInformation.__slots__ = [
    'information',
]
struct_HFInspireFaceExtendedInformation._fields_ = [
    ('information', HChar * int(256)),
]

HFInspireFaceExtendedInformation = struct_HFInspireFaceExtendedInformation# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 895

PHFInspireFaceExtendedInformation = POINTER(struct_HFInspireFaceExtendedInformation)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 895

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 902
if _libs[_LIBRARY_FILENAME].has("HFQueryInspireFaceExtendedInformation", "cdecl"):
    HFQueryInspireFaceExtendedInformation = _libs[_LIBRARY_FILENAME].get("HFQueryInspireFaceExtendedInformation", "cdecl")
    HFQueryInspireFaceExtendedInformation.argtypes = [PHFInspireFaceExtendedInformation]
    HFQueryInspireFaceExtendedInformation.restype = HResult

enum_HFLogLevel = c_int# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_NONE = 0# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_DEBUG = (HF_LOG_NONE + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_INFO = (HF_LOG_DEBUG + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_WARN = (HF_LOG_INFO + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_ERROR = (HF_LOG_WARN + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HF_LOG_FATAL = (HF_LOG_ERROR + 1)# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

HFLogLevel = enum_HFLogLevel# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 914

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 919
if _libs[_LIBRARY_FILENAME].has("HFSetLogLevel", "cdecl"):
    HFSetLogLevel = _libs[_LIBRARY_FILENAME].get("HFSetLogLevel", "cdecl")
    HFSetLogLevel.argtypes = [HFLogLevel]
    HFSetLogLevel.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 924
if _libs[_LIBRARY_FILENAME].has("HFLogDisable", "cdecl"):
    HFLogDisable = _libs[_LIBRARY_FILENAME].get("HFLogDisable", "cdecl")
    HFLogDisable.argtypes = []
    HFLogDisable.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 937
if _libs[_LIBRARY_FILENAME].has("HFDeBugImageStreamImShow", "cdecl"):
    HFDeBugImageStreamImShow = _libs[_LIBRARY_FILENAME].get("HFDeBugImageStreamImShow", "cdecl")
    HFDeBugImageStreamImShow.argtypes = [HFImageStream]
    HFDeBugImageStreamImShow.restype = None

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 949
if _libs[_LIBRARY_FILENAME].has("HFDeBugImageStreamDecodeSave", "cdecl"):
    HFDeBugImageStreamDecodeSave = _libs[_LIBRARY_FILENAME].get("HFDeBugImageStreamDecodeSave", "cdecl")
    HFDeBugImageStreamDecodeSave.argtypes = [HFImageStream, HPath]
    HFDeBugImageStreamDecodeSave.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 964
if _libs[_LIBRARY_FILENAME].has("HFDeBugShowResourceStatistics", "cdecl"):
    HFDeBugShowResourceStatistics = _libs[_LIBRARY_FILENAME].get("HFDeBugShowResourceStatistics", "cdecl")
    HFDeBugShowResourceStatistics.argtypes = []
    HFDeBugShowResourceStatistics.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 974
if _libs[_LIBRARY_FILENAME].has("HFDeBugGetUnreleasedSessionsCount", "cdecl"):
    HFDeBugGetUnreleasedSessionsCount = _libs[_LIBRARY_FILENAME].get("HFDeBugGetUnreleasedSessionsCount", "cdecl")
    HFDeBugGetUnreleasedSessionsCount.argtypes = [POINTER(HInt32)]
    HFDeBugGetUnreleasedSessionsCount.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 985
if _libs[_LIBRARY_FILENAME].has("HFDeBugGetUnreleasedSessions", "cdecl"):
    HFDeBugGetUnreleasedSessions = _libs[_LIBRARY_FILENAME].get("HFDeBugGetUnreleasedSessions", "cdecl")
    HFDeBugGetUnreleasedSessions.argtypes = [POINTER(HFSession), HInt32]
    HFDeBugGetUnreleasedSessions.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 995
if _libs[_LIBRARY_FILENAME].has("HFDeBugGetUnreleasedStreamsCount", "cdecl"):
    HFDeBugGetUnreleasedStreamsCount = _libs[_LIBRARY_FILENAME].get("HFDeBugGetUnreleasedStreamsCount", "cdecl")
    HFDeBugGetUnreleasedStreamsCount.argtypes = [POINTER(HInt32)]
    HFDeBugGetUnreleasedStreamsCount.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 1006
if _libs[_LIBRARY_FILENAME].has("HFDeBugGetUnreleasedStreams", "cdecl"):
    HFDeBugGetUnreleasedStreams = _libs[_LIBRARY_FILENAME].get("HFDeBugGetUnreleasedStreams", "cdecl")
    HFDeBugGetUnreleasedStreams.argtypes = [POINTER(HFImageStream), HInt32]
    HFDeBugGetUnreleasedStreams.restype = HResult

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 27
try:
    HF_ENABLE_NONE = 0x00000000
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 28
try:
    HF_ENABLE_FACE_RECOGNITION = 0x00000002
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 29
try:
    HF_ENABLE_LIVENESS = 0x00000004
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 30
try:
    HF_ENABLE_IR_LIVENESS = 0x00000008
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 31
try:
    HF_ENABLE_MASK_DETECT = 0x00000010
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 32
try:
    HF_ENABLE_FACE_ATTRIBUTE = 0x00000020
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 33
try:
    HF_ENABLE_PLACEHOLDER_ = 0x00000040
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 34
try:
    HF_ENABLE_QUALITY = 0x00000080
except:
    pass

# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 35
try:
    HF_ENABLE_INTERACTION = 0x00000100
except:
    pass

HFImageData = struct_HFImageData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 71

HFImageBitmapData = struct_HFImageBitmapData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 102

HFSessionCustomParameter = struct_HFSessionCustomParameter# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 259

HFFaceBasicToken = struct_HFFaceBasicToken# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 324

HFFaceEulerAngle = struct_HFFaceEulerAngle# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 335

HFMultipleFaceData = struct_HFMultipleFaceData# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 350

HFFaceFeature = struct_HFFaceFeature# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 451

HFFeatureHubConfiguration = struct_HFFeatureHubConfiguration# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 519

HFFaceFeatureIdentity = struct_HFFaceFeatureIdentity# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 548

HFSearchTopKResults = struct_HFSearchTopKResults# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 557

HFFeatureHubExistingIds = struct_HFFeatureHubExistingIds# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 665

HFRGBLivenessConfidence = struct_HFRGBLivenessConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 717

HFFaceMaskConfidence = struct_HFFaceMaskConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 740

HFFaceQualityConfidence = struct_HFFaceQualityConfidence# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 763

HFFaceInteractionState = struct_HFFaceInteractionState# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 798

HFFaceInteractionsActions = struct_HFFaceInteractionsActions# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 817

HFFaceAttributeResult = struct_HFFaceAttributeResult# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 852

HFInspireFaceVersion = struct_HFInspireFaceVersion# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 877

HFInspireFaceExtendedInformation = struct_HFInspireFaceExtendedInformation# /Users/tunm/work/InspireFace/cpp/inspireface/c_api/inspireface.h: 895

# No inserted files

# No prefix-stripping

