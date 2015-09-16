/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxesPlane2DActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridAxesPlane2DActor - renders a 2D grid for vtkGridAxes2DActor.
// .SECTION Description
// vtkGridAxesPlane2DActor is designed for use by vtkGridAxes2DActor to render
// the wireframe for the grid plane. It can also be used directly to render such
// a wireframe in a renderer.
#pragma once

#include "vtkProp3D.h"

#include "vtkGridAxesHelper.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include <deque>

#include <core/core_api.h>


class vtkActor;
class vtkCellArray;
class vtkDoubleArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProperty;

class CORE_API vtkGridAxesPlane2DActor : public vtkProp3D
{
public:
  static vtkGridAxesPlane2DActor* New();
  vtkTypeMacro(vtkGridAxesPlane2DActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the bounding box defining the grid space. This, together with the
  // \c Face identify which planar surface this class is interested in. This
  // class is designed to work with a single planar surface.
  // Note: this is only needed/used when the vtkGridAxesHelper is not provided
  // when calling New(), otherwise the vtkGridAxesHelper is assumed to be
  // initialized externally.
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);

  // These are in the same order as the faces of a vtkVoxel.
  enum Faces
    {
    MIN_YZ = vtkGridAxesHelper::MIN_YZ,
    MIN_ZX = vtkGridAxesHelper::MIN_ZX,
    MIN_XY = vtkGridAxesHelper::MIN_XY,
    MAX_YZ = vtkGridAxesHelper::MAX_YZ,
    MAX_ZX = vtkGridAxesHelper::MAX_ZX,
    MAX_XY = vtkGridAxesHelper::MAX_XY
    };

  // Description:
  // Indicate which face of the specified bounds is this class operating with.
  // Note: this is only needed/used when the vtkGridAxesHelper is not provided
  // when calling New(), otherwise the vtkGridAxesHelper is assumed to be
  // initialized externally.
  vtkSetClampMacro(Face, int, MIN_YZ, MAX_XY);
  vtkGetMacro(Face, int);

  // Description:
  // Get/Set whether to generate lines for the plane's grid. Default is true.
  vtkSetMacro(GenerateGrid, bool);
  vtkGetMacro(GenerateGrid, bool);
  vtkBooleanMacro(GenerateGrid, bool);

  // Description:
  // Get/Set whether to generate the polydata for the plane's edges. Default is
  // true.
  vtkSetMacro(GenerateEdges, bool);
  vtkGetMacro(GenerateEdges, bool);
  vtkBooleanMacro(GenerateEdges, bool);

  // Description:
  // Get/Set whether to generate tick markers for the tick positions. Default is
  // true.
  vtkSetMacro(GenerateTicks, bool);
  vtkGetMacro(GenerateTicks, bool);
  vtkBooleanMacro(GenerateTicks, bool);

  enum
    {
    TICK_DIRECTION_INWARDS=0x1,
    TICK_DIRECTION_OUTWARDS=0x2,
    TICK_DIRECTION_BOTH=TICK_DIRECTION_INWARDS|TICK_DIRECTION_OUTWARDS,
    };

  // Description:
  // Get/Set the tick direction.
  vtkSetClampMacro(TickDirection, unsigned int,
    static_cast<unsigned int>(TICK_DIRECTION_INWARDS),
    static_cast<unsigned int>(TICK_DIRECTION_BOTH));
  vtkGetMacro(TickDirection, unsigned int);

  // Description:
  // Set the tick positions for each of the coordinate axis. Which tick
  // positions get used depended on the face being rendered e.g. if Face is
  // MIN_XY, then the tick positions for Z-axis i.e. axis=2 will not be used
  // and hence need not be specified. Pass NULL for data will clear the ticks
  // positions for that axis.
  // Note: This creates a deep-copy of the values in \c data and stores that.
  void SetTickPositions(int axis, vtkDoubleArray* data);
  const std::deque<double>& GetTickPositions(int axis)
    { return (axis>=0 && axis<3)? this->TickPositions[axis] : this->EmptyVector; }

  // Description:
  // Enable/Disable layer support. Default is off. When enabled, the prop will
  // only render when the viewport's layer matches the Layer set on this prop.
  vtkSetMacro(EnableLayerSupport, bool);
  vtkGetMacro(EnableLayerSupport, bool);
  vtkBooleanMacro(EnableLayerSupport, bool);

  // Description:
  // Set the layer to render this prop under when EnableLayerSupport is true.
  // Default is 0.
  vtkSetMacro(Layer, int);
  vtkGetMacro(Layer, int);

  // Description:
  // Get/Set the property used to control the appearance of the rendered grid.
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  // Description:
  // Returns the prop bounds.
  virtual double *GetBounds()
    {
    this->GetGridBounds(this->Bounds);
    return this->Bounds;
    }

  virtual int RenderOpaqueGeometry(vtkViewport *);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  virtual int RenderOverlay(vtkViewport* viewport);
  virtual int HasTranslucentPolygonalGeometry();
  virtual void ReleaseGraphicsResources(vtkWindow *);

//BTX
protected:
  vtkGridAxesPlane2DActor(vtkGridAxesHelper* helper=NULL);
  ~vtkGridAxesPlane2DActor();

  // Description:
  // vtkGridAxes2DActor uses this method to create vtkGridAxesPlane2DActor
  // instance. In that case, vtkGridAxesPlane2DActor assumes that the
  // vtkGridAxesHelper will be updated and initialized externally. That avoids
  // unnecessary duplicate computations per render.
  static vtkGridAxesPlane2DActor* New(vtkGridAxesHelper* helper);
  friend class vtkGridAxes2DActor;

  // Description:
  // Returns true if the actor must render in the viewport.
  bool IsLayerActive(vtkViewport* viewport);

  // Description:
  // Update's the polydata.
  void Update(vtkViewport* viewport);
  bool UpdateEdges(vtkViewport* viewport);
  bool UpdateGrid(vtkViewport* viewport);
  bool UpdateTicks(vtkViewport* viewport);

  double GridBounds[6];
  int Face;

  bool GenerateGrid;
  bool GenerateEdges;
  bool GenerateTicks;
  unsigned int TickDirection;
  std::deque<double> TickPositions[3];

  bool EnableLayerSupport;
  int Layer;

  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPoints> PolyDataPoints;
  vtkNew<vtkCellArray> PolyDataLines;
  vtkNew<vtkPolyDataMapper> Mapper;
  vtkNew<vtkActor> Actor;

  vtkSmartPointer<vtkGridAxesHelper> Helper;
  bool HelperManagedExternally;

private:
  vtkGridAxesPlane2DActor(const vtkGridAxesPlane2DActor&); // Not implemented.
  void operator=(const vtkGridAxesPlane2DActor&); // Not implemented.
  std::deque<double> EmptyVector;

  typedef std::pair<vtkVector3d, vtkVector3d> LineSegmentType;
  std::deque<LineSegmentType> LineSegments;

//ETX
};
