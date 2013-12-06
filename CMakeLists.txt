cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project (VTKTryIt)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkInteractionWidgets
  vtkRenderingCore
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone Cone.cxx)
target_link_libraries(Cone ${VTK_LIBRARIES})
