__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403
import platform

def get_library_suffix():
    system = platform.system()

    if system == 'Windows':
        return 'dll'
    elif system == 'Darwin':  # macOS
        return 'dylib'
    else:  # Linux and other Unix-like systems
        return 'so'

library_suffix = get_library_suffix()

_LIBRARY_FILENAME = f"libInspireFace." + library_suffix

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
            except Exception:  # pylint: disable=broad-except
                pass

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
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

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

HPVoid = POINTER(None)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 8

HImageHandle = POINTER(None)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 9

HContextHandle = POINTER(None)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 10

HFloat = c_float# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 12

HPFloat = POINTER(c_float)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 13

HInt32 = c_int# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 16

HPInt32 = POINTER(c_int)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 17

HResult = c_long# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 18

HString = String# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 19

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 27
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

HFaceRect = struct_HFaceRect# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/intypedef.h: 27

enum_HF_ImageFormat = c_int# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_RGB = 0# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_BGR = 1# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_RGBA = 2# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_BGRA = 3# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_YUV_NV12 = 4# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

STREAM_YUV_NV21 = 5# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

HF_ImageFormat = enum_HF_ImageFormat# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 48

enum_HF_Rotation = c_int# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

CAMERA_ROTATION_0 = 0# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

CAMERA_ROTATION_90 = 1# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

CAMERA_ROTATION_180 = 2# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

CAMERA_ROTATION_270 = 3# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

HF_Rotation = enum_HF_Rotation# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 60

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 73
class struct_HF_ImageData(Structure):
    pass

struct_HF_ImageData.__slots__ = [
    'data',
    'width',
    'height',
    'format',
    'rotation',
]
struct_HF_ImageData._fields_ = [
    ('data', POINTER(c_uint8)),
    ('width', HInt32),
    ('height', HInt32),
    ('format', HF_ImageFormat),
    ('rotation', HF_Rotation),
]

HF_ImageData = struct_HF_ImageData# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 73

Ptr_HF_ImageData = POINTER(struct_HF_ImageData)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 73

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 87
if _libs[_LIBRARY_FILENAME].has("HF_CreateImageStream", "cdecl"):
    HF_CreateImageStream = _libs[_LIBRARY_FILENAME].get("HF_CreateImageStream", "cdecl")
    HF_CreateImageStream.argtypes = [Ptr_HF_ImageData, POINTER(HImageHandle)]
    HF_CreateImageStream.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 98
if _libs[_LIBRARY_FILENAME].has("HF_ReleaseImageStream", "cdecl"):
    HF_ReleaseImageStream = _libs[_LIBRARY_FILENAME].get("HF_ReleaseImageStream", "cdecl")
    HF_ReleaseImageStream.argtypes = [HImageHandle]
    HF_ReleaseImageStream.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 121
class struct_HF_ContextCustomParameter(Structure):
    pass

struct_HF_ContextCustomParameter.__slots__ = [
    'enable_recognition',
    'enable_liveness',
    'enable_ir_liveness',
    'enable_mask_detect',
    'enable_age',
    'enable_gender',
    'enable_face_quality',
    'enable_interaction_liveness',
]
struct_HF_ContextCustomParameter._fields_ = [
    ('enable_recognition', HInt32),
    ('enable_liveness', HInt32),
    ('enable_ir_liveness', HInt32),
    ('enable_mask_detect', HInt32),
    ('enable_age', HInt32),
    ('enable_gender', HInt32),
    ('enable_face_quality', HInt32),
    ('enable_interaction_liveness', HInt32),
]

HF_ContextCustomParameter = struct_HF_ContextCustomParameter# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 121

Ptr_HF_ContextCustomParameter = POINTER(struct_HF_ContextCustomParameter)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 121

enum_HF_DetectMode = c_int# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 130

HF_DETECT_MODE_IMAGE = 0# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 130

HF_DETECT_MODE_VIDEO = (HF_DETECT_MODE_IMAGE + 1)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 130

HF_DetectMode = enum_HF_DetectMode# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 130

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 142
if _libs[_LIBRARY_FILENAME].has("HF_CreateFaceContextFromResourceFile", "cdecl"):
    HF_CreateFaceContextFromResourceFile = _libs[_LIBRARY_FILENAME].get("HF_CreateFaceContextFromResourceFile", "cdecl")
    HF_CreateFaceContextFromResourceFile.argtypes = [HString, HF_ContextCustomParameter, HF_DetectMode, HInt32, POINTER(HContextHandle)]
    HF_CreateFaceContextFromResourceFile.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 160
if _libs[_LIBRARY_FILENAME].has("HF_CreateFaceContextFromResourceFileOptional", "cdecl"):
    HF_CreateFaceContextFromResourceFileOptional = _libs[_LIBRARY_FILENAME].get("HF_CreateFaceContextFromResourceFileOptional", "cdecl")
    HF_CreateFaceContextFromResourceFileOptional.argtypes = [HString, HInt32, HF_DetectMode, HInt32, POINTER(HContextHandle)]
    HF_CreateFaceContextFromResourceFileOptional.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 176
class struct_HF_DatabaseConfiguration(Structure):
    pass

struct_HF_DatabaseConfiguration.__slots__ = [
    'enableUseDb',
    'dbPath',
]
struct_HF_DatabaseConfiguration._fields_ = [
    ('enableUseDb', HInt32),
    ('dbPath', HString),
]

HF_DatabaseConfiguration = struct_HF_DatabaseConfiguration# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 176

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 187
if _libs[_LIBRARY_FILENAME].has("HF_FaceContextDataPersistence", "cdecl"):
    HF_FaceContextDataPersistence = _libs[_LIBRARY_FILENAME].get("HF_FaceContextDataPersistence", "cdecl")
    HF_FaceContextDataPersistence.argtypes = [HContextHandle, HF_DatabaseConfiguration]
    HF_FaceContextDataPersistence.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 195
if _libs[_LIBRARY_FILENAME].has("HF_ReleaseFaceContext", "cdecl"):
    HF_ReleaseFaceContext = _libs[_LIBRARY_FILENAME].get("HF_ReleaseFaceContext", "cdecl")
    HF_ReleaseFaceContext.argtypes = [HContextHandle]
    HF_ReleaseFaceContext.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 205
class struct_HF_FaceBasicToken(Structure):
    pass

struct_HF_FaceBasicToken.__slots__ = [
    'size',
    'data',
]
struct_HF_FaceBasicToken._fields_ = [
    ('size', HInt32),
    ('data', HPVoid),
]

HF_FaceBasicToken = struct_HF_FaceBasicToken# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 205

Ptr_HF_FaceBasicToken = POINTER(struct_HF_FaceBasicToken)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 205

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 216
class struct_HF_FaceEulerAngle(Structure):
    pass

struct_HF_FaceEulerAngle.__slots__ = [
    'roll',
    'yaw',
    'pitch',
]
struct_HF_FaceEulerAngle._fields_ = [
    ('roll', POINTER(HFloat)),
    ('yaw', POINTER(HFloat)),
    ('pitch', POINTER(HFloat)),
]

HF_FaceEulerAngle = struct_HF_FaceEulerAngle# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 216

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 230
class struct_HF_MultipleFaceData(Structure):
    pass

struct_HF_MultipleFaceData.__slots__ = [
    'detectedNum',
    'rects',
    'trackIds',
    'angles',
    'tokens',
]
struct_HF_MultipleFaceData._fields_ = [
    ('detectedNum', HInt32),
    ('rects', POINTER(HFaceRect)),
    ('trackIds', POINTER(HInt32)),
    ('angles', HF_FaceEulerAngle),
    ('tokens', Ptr_HF_FaceBasicToken),
]

HF_MultipleFaceData = struct_HF_MultipleFaceData# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 230

Ptr_HF_MultipleFaceData = POINTER(struct_HF_MultipleFaceData)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 230

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 239
if _libs[_LIBRARY_FILENAME].has("HF_FaceContextSetTrackPreviewSize", "cdecl"):
    HF_FaceContextSetTrackPreviewSize = _libs[_LIBRARY_FILENAME].get("HF_FaceContextSetTrackPreviewSize", "cdecl")
    HF_FaceContextSetTrackPreviewSize.argtypes = [HContextHandle, HInt32]
    HF_FaceContextSetTrackPreviewSize.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 250
if _libs[_LIBRARY_FILENAME].has("HF_FaceContextRunFaceTrack", "cdecl"):
    HF_FaceContextRunFaceTrack = _libs[_LIBRARY_FILENAME].get("HF_FaceContextRunFaceTrack", "cdecl")
    HF_FaceContextRunFaceTrack.argtypes = [HContextHandle, HImageHandle, Ptr_HF_MultipleFaceData]
    HF_FaceContextRunFaceTrack.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 264
class struct_HF_FaceFeature(Structure):
    pass

struct_HF_FaceFeature.__slots__ = [
    'size',
    'data',
]
struct_HF_FaceFeature._fields_ = [
    ('size', HInt32),
    ('data', HPFloat),
]

HF_FaceFeature = struct_HF_FaceFeature# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 264

Ptr_HF_FaceFeature = POINTER(struct_HF_FaceFeature)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 264

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 275
class struct_HF_FaceFeatureIdentity(Structure):
    pass

struct_HF_FaceFeatureIdentity.__slots__ = [
    'customId',
    'tag',
    'feature',
]
struct_HF_FaceFeatureIdentity._fields_ = [
    ('customId', HInt32),
    ('tag', HString),
    ('feature', Ptr_HF_FaceFeature),
]

HF_FaceFeatureIdentity = struct_HF_FaceFeatureIdentity# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 275

Ptr_HF_FaceFeatureIdentity = POINTER(struct_HF_FaceFeatureIdentity)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 275

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 287
if _libs[_LIBRARY_FILENAME].has("HF_FaceFeatureExtract", "cdecl"):
    HF_FaceFeatureExtract = _libs[_LIBRARY_FILENAME].get("HF_FaceFeatureExtract", "cdecl")
    HF_FaceFeatureExtract.argtypes = [HContextHandle, HImageHandle, HF_FaceBasicToken, Ptr_HF_FaceFeature]
    HF_FaceFeatureExtract.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 300
if _libs[_LIBRARY_FILENAME].has("HF_FaceFeatureExtractCpy", "cdecl"):
    HF_FaceFeatureExtractCpy = _libs[_LIBRARY_FILENAME].get("HF_FaceFeatureExtractCpy", "cdecl")
    HF_FaceFeatureExtractCpy.argtypes = [HContextHandle, HImageHandle, HF_FaceBasicToken, HPFloat]
    HF_FaceFeatureExtractCpy.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 313
if _libs[_LIBRARY_FILENAME].has("HF_FaceComparison1v1", "cdecl"):
    HF_FaceComparison1v1 = _libs[_LIBRARY_FILENAME].get("HF_FaceComparison1v1", "cdecl")
    HF_FaceComparison1v1.argtypes = [HContextHandle, HF_FaceFeature, HF_FaceFeature, HPFloat]
    HF_FaceComparison1v1.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 322
if _libs[_LIBRARY_FILENAME].has("HF_GetFeatureLength", "cdecl"):
    HF_GetFeatureLength = _libs[_LIBRARY_FILENAME].get("HF_GetFeatureLength", "cdecl")
    HF_GetFeatureLength.argtypes = [HContextHandle, HPInt32]
    HF_GetFeatureLength.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 333
if _libs[_LIBRARY_FILENAME].has("HF_FeaturesGroupInsertFeature", "cdecl"):
    HF_FeaturesGroupInsertFeature = _libs[_LIBRARY_FILENAME].get("HF_FeaturesGroupInsertFeature", "cdecl")
    HF_FeaturesGroupInsertFeature.argtypes = [HContextHandle, HF_FaceFeatureIdentity]
    HF_FeaturesGroupInsertFeature.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 345
if _libs[_LIBRARY_FILENAME].has("HF_FeaturesGroupFeatureSearch", "cdecl"):
    HF_FeaturesGroupFeatureSearch = _libs[_LIBRARY_FILENAME].get("HF_FeaturesGroupFeatureSearch", "cdecl")
    HF_FeaturesGroupFeatureSearch.argtypes = [HContextHandle, HF_FaceFeature, HPFloat, Ptr_HF_FaceFeatureIdentity]
    HF_FeaturesGroupFeatureSearch.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 355
if _libs[_LIBRARY_FILENAME].has("HF_FeaturesGroupFeatureRemove", "cdecl"):
    HF_FeaturesGroupFeatureRemove = _libs[_LIBRARY_FILENAME].get("HF_FeaturesGroupFeatureRemove", "cdecl")
    HF_FeaturesGroupFeatureRemove.argtypes = [HContextHandle, HInt32]
    HF_FeaturesGroupFeatureRemove.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 365
if _libs[_LIBRARY_FILENAME].has("HF_FeaturesGroupFeatureUpdate", "cdecl"):
    HF_FeaturesGroupFeatureUpdate = _libs[_LIBRARY_FILENAME].get("HF_FeaturesGroupFeatureUpdate", "cdecl")
    HF_FeaturesGroupFeatureUpdate.argtypes = [HContextHandle, HF_FaceFeatureIdentity]
    HF_FeaturesGroupFeatureUpdate.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 376
if _libs[_LIBRARY_FILENAME].has("HF_FeaturesGroupGetFeatureIdentity", "cdecl"):
    HF_FeaturesGroupGetFeatureIdentity = _libs[_LIBRARY_FILENAME].get("HF_FeaturesGroupGetFeatureIdentity", "cdecl")
    HF_FeaturesGroupGetFeatureIdentity.argtypes = [HContextHandle, HInt32, Ptr_HF_FaceFeatureIdentity]
    HF_FeaturesGroupGetFeatureIdentity.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 385
if _libs[_LIBRARY_FILENAME].has("HF_FeatureGroupGetCount", "cdecl"):
    HF_FeatureGroupGetCount = _libs[_LIBRARY_FILENAME].get("HF_FeatureGroupGetCount", "cdecl")
    HF_FeatureGroupGetCount.argtypes = [HContextHandle, POINTER(HInt32)]
    HF_FeatureGroupGetCount.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 393
if _libs[_LIBRARY_FILENAME].has("HF_ViewFaceDBTable", "cdecl"):
    HF_ViewFaceDBTable = _libs[_LIBRARY_FILENAME].get("HF_ViewFaceDBTable", "cdecl")
    HF_ViewFaceDBTable.argtypes = [HContextHandle]
    HF_ViewFaceDBTable.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 412
if _libs[_LIBRARY_FILENAME].has("HF_MultipleFacePipelineProcess", "cdecl"):
    HF_MultipleFacePipelineProcess = _libs[_LIBRARY_FILENAME].get("HF_MultipleFacePipelineProcess", "cdecl")
    HF_MultipleFacePipelineProcess.argtypes = [HContextHandle, HImageHandle, Ptr_HF_MultipleFaceData, HF_ContextCustomParameter]
    HF_MultipleFacePipelineProcess.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 428
if _libs[_LIBRARY_FILENAME].has("HF_MultipleFacePipelineProcessOptional", "cdecl"):
    HF_MultipleFacePipelineProcessOptional = _libs[_LIBRARY_FILENAME].get("HF_MultipleFacePipelineProcessOptional", "cdecl")
    HF_MultipleFacePipelineProcessOptional.argtypes = [HContextHandle, HImageHandle, Ptr_HF_MultipleFaceData, HInt32]
    HF_MultipleFacePipelineProcessOptional.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 440
class struct_HF_RGBLivenessConfidence(Structure):
    pass

struct_HF_RGBLivenessConfidence.__slots__ = [
    'num',
    'confidence',
]
struct_HF_RGBLivenessConfidence._fields_ = [
    ('num', HInt32),
    ('confidence', HPFloat),
]

HF_RGBLivenessConfidence = struct_HF_RGBLivenessConfidence# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 440

Ptr_HF_RGBLivenessConfidence = POINTER(struct_HF_RGBLivenessConfidence)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 440

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 452
if _libs[_LIBRARY_FILENAME].has("HF_FaceRecognitionThresholdSetting", "cdecl"):
    HF_FaceRecognitionThresholdSetting = _libs[_LIBRARY_FILENAME].get("HF_FaceRecognitionThresholdSetting", "cdecl")
    HF_FaceRecognitionThresholdSetting.argtypes = [HContextHandle, c_float]
    HF_FaceRecognitionThresholdSetting.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 465
if _libs[_LIBRARY_FILENAME].has("HF_GetRGBLivenessConfidence", "cdecl"):
    HF_GetRGBLivenessConfidence = _libs[_LIBRARY_FILENAME].get("HF_GetRGBLivenessConfidence", "cdecl")
    HF_GetRGBLivenessConfidence.argtypes = [HContextHandle, Ptr_HF_RGBLivenessConfidence]
    HF_GetRGBLivenessConfidence.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 476
class struct_HF_FaceMaskConfidence(Structure):
    pass

struct_HF_FaceMaskConfidence.__slots__ = [
    'num',
    'confidence',
]
struct_HF_FaceMaskConfidence._fields_ = [
    ('num', HInt32),
    ('confidence', HPFloat),
]

HF_FaceMaskConfidence = struct_HF_FaceMaskConfidence# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 476

Ptr_HF_FaceMaskConfidence = POINTER(struct_HF_FaceMaskConfidence)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 476

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 489
if _libs[_LIBRARY_FILENAME].has("HF_GetFaceMaskConfidence", "cdecl"):
    HF_GetFaceMaskConfidence = _libs[_LIBRARY_FILENAME].get("HF_GetFaceMaskConfidence", "cdecl")
    HF_GetFaceMaskConfidence.argtypes = [HContextHandle, Ptr_HF_FaceMaskConfidence]
    HF_GetFaceMaskConfidence.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 502
if _libs[_LIBRARY_FILENAME].has("HF_FaceQualityDetect", "cdecl"):
    HF_FaceQualityDetect = _libs[_LIBRARY_FILENAME].get("HF_FaceQualityDetect", "cdecl")
    HF_FaceQualityDetect.argtypes = [HContextHandle, HF_FaceBasicToken, POINTER(HFloat)]
    HF_FaceQualityDetect.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 515
class struct_HF_InspireFaceVersion(Structure):
    pass

struct_HF_InspireFaceVersion.__slots__ = [
    'major',
    'minor',
    'patch',
]
struct_HF_InspireFaceVersion._fields_ = [
    ('major', c_int),
    ('minor', c_int),
    ('patch', c_int),
]

HF_InspireFaceVersion = struct_HF_InspireFaceVersion# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 515

Ptr_HF_InspireFaceVersion = POINTER(struct_HF_InspireFaceVersion)# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 515

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 525
if _libs[_LIBRARY_FILENAME].has("HF_QueryInspireFaceVersion", "cdecl"):
    HF_QueryInspireFaceVersion = _libs[_LIBRARY_FILENAME].get("HF_QueryInspireFaceVersion", "cdecl")
    HF_QueryInspireFaceVersion.argtypes = [Ptr_HF_InspireFaceVersion]
    HF_QueryInspireFaceVersion.restype = HResult

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 538
if _libs[_LIBRARY_FILENAME].has("HF_DeBugImageStreamImShow", "cdecl"):
    HF_DeBugImageStreamImShow = _libs[_LIBRARY_FILENAME].get("HF_DeBugImageStreamImShow", "cdecl")
    HF_DeBugImageStreamImShow.argtypes = [HImageHandle]
    HF_DeBugImageStreamImShow.restype = None

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 27
try:
    HF_ENABLE_FACE_RECOGNITION = 2
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 28
try:
    HF_ENABLE_LIVENESS = 4
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 29
try:
    HF_ENABLE_IR_LIVENESS = 8
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 30
try:
    HF_ENABLE_MASK_DETECT = 16
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 31
try:
    HF_ENABLE_AGE_PREDICT = 32
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 32
try:
    HF_ENABLE_GENDER_PREDICT = 64
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 33
try:
    HF_ENABLE_QUALITY = 128
except:
    pass

# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 34
try:
    HF_ENABLE_INTERACTION = 256
except:
    pass

HF_ImageData = struct_HF_ImageData# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 73

HF_ContextCustomParameter = struct_HF_ContextCustomParameter# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 121

HF_DatabaseConfiguration = struct_HF_DatabaseConfiguration# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 176

HF_FaceBasicToken = struct_HF_FaceBasicToken# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 205

HF_FaceEulerAngle = struct_HF_FaceEulerAngle# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 216

HF_MultipleFaceData = struct_HF_MultipleFaceData# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 230

HF_FaceFeature = struct_HF_FaceFeature# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 264

HF_FaceFeatureIdentity = struct_HF_FaceFeatureIdentity# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 275

HF_RGBLivenessConfidence = struct_HF_RGBLivenessConfidence# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 440

HF_FaceMaskConfidence = struct_HF_FaceMaskConfidence# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 476

HF_InspireFaceVersion = struct_HF_InspireFaceVersion# /Users/tunm/work/HyperFace/cpp/inspireface/c_api/inspireface.h: 515

# No inserted files

# No prefix-stripping

