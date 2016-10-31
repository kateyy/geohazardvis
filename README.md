geohazardvis
=======

### Dependencies

The following dev-libraries and programs need to be provided for correct CMake configuration:
* C++11 compatible compiler (minimum: Windows: Visual Studio 2013, Linux: gcc 4.9)
* CMake (>= 2.8.12): http://www.cmake.org/
* Qt5 (>= 5.3): http://www.qt-project.org/
* VTK (>= 7.1): http://www.vtk.org/

The following dependencies can automatically be built within the CMake project:
* libzeug (commit aa34e42): https://github.com/kateyy/libzeug (forked from https://github.com/cginternals/libzeug)
* Google Test (1.8, required for tests only): https://github.com/google/googletest
