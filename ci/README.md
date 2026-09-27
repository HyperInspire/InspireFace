# CI/CD

CI clones `https://github.com/tunmx/inspireface-3rdparty.git` with
`--recurse-submodules`. Dependency versions are selected by that repository's
submodule commits. InspireCV contains the compiler compatibility fixes upstream;
InspireFace does not apply a local dependency patch.

Build scripts stop at the first failure. Docker Compose jobs use
`--exit-code-from` so a failed container also fails its Actions step.

The manylinux2014 wheel scripts use Python 3.12 by default, with PEP 517 build
isolation for the dependencies in `python/pyproject.toml`. Override
`ISF_WHEEL_PYTHON` to select another installed interpreter. The ctypes package
produces one `py3-none` wheel per platform; building that wheel does not replace
testing installation and imports on each supported Python version.
