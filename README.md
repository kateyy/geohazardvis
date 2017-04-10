geohazardvis
=======

### Dependencies

The following dev-libraries and programs need to be provided for correct CMake configuration:
* C++11 compatible compiler (minimum: Windows: Visual Studio 2013, Linux: gcc 4.9)
* CMake (>= 3.3): http://www.cmake.org/
* Qt5 (>= 5.5): http://www.qt.io/
* VTK (>= 7.1): http://www.vtk.org/

The following dependencies can automatically be built within the CMake project:
* libzeug (branch geohazardvis_2016_06): https://github.com/kateyy/libzeug (forked from https://github.com/cginternals/libzeug)
* Google Test (1.8, required for tests only): https://github.com/google/googletest
