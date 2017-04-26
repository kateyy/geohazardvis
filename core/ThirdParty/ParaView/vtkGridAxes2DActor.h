/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxes2DActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGridAxes2DActor
 *
*/

#ifndef vtkGridAxes2DActor_h
#define vtkGridAxes2DActor_h

#include "vtkProp3D.h"

#include "vtkGridAxesHelper.h"       // needed of Helper
#include "vtkGridAxesPlane2DActor.h" // needed for inline methods
#include "vtkNew.h"                  // needed for vtkNew.
#include "vtkSmartPointer.h"         // needed for vtkSmartPointer.
#include "vtkStdString.h"            // needed for vtkStdString.

#include <core/core_api.h>

class vtkAxis;
class vtkContextScene;
class vtkDoubleArray;
class vtkProperty;
class vtkTextProperty;

class CORE_API vtkGridAxes2DActor : public vtkProp3D
{
public:
  static vtkGridAxes2DActor* New();
  vtkTypeMacro(vtkGridAxes2DActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // XXX GeohazardVis
  // Line colors are not set by the Property, but are set separately here for edges and grid lines.
  void GetEdgeColor(unsigned char edgeColor[3]) const;
  unsigned char * GetEdgeColor() const;
  void SetEdgeColor(unsigned char edgeColor[3]);
  void SetEdgeColor(unsigned char r, unsigned char g, unsigned char b);
  void GetGridLineColor(unsigned char gridLineColor[3]) const;
  unsigned char * GetGridLineColor() const;
  void SetGridLineColor(unsigned char gridLineColor[3]);
  void SetGridLineColor(unsigned char r, unsigned char g, unsigned char b);
  // XXX End GeohazardVis

  //@{
  /**
   * Set the bounding box defining the grid space. This, together with the
   * \c Face identify which planar surface this class is interested in. This
   * class is designed to work with a single planar surface.
   */
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);
  //@}

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

  //@{
  /**
   * Indicate which face of the specified bounds is this class operating with.
   */
  vtkSetClampMacro(Face, int, MIN_YZ, MAX_XY);
  vtkGetMacro(Face, int);
  //@}

  /**
   * Valid values for LabelMask.
   */
  enum LabelMasks
  {
    MIN_X = vtkGridAxesHelper::MIN_X,
    MIN_Y = vtkGridAxesHelper::MIN_Y,
    MIN_Z = vtkGridAxesHelper::MIN_Z,
    MAX_X = vtkGridAxesHelper::MAX_X,
    MAX_Y = vtkGridAxesHelper::MAX_Y,
    MAX_Z = vtkGridAxesHelper::MAX_Z
  };

  //@{
  /**
   * Set the axes to label.
   */
  vtkSetMacro(LabelMask, unsigned int);
  vtkGetMacro(LabelMask, unsigned int);
  //@}

  //@{
  /**
   * Get/Set the property used to control the appearance of the rendered grid.
   */
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();
  //@}

  //@{
  /**
   * Get/Set the title text properties for each of the coordinate axes. Which
   * properties will be used depends on the selected Face being rendered.
   */
  void SetTitleTextProperty(int axis, vtkTextProperty*);
  vtkTextProperty* GetTitleTextProperty(int axis);
  //@}

  //@{
  /**
   * Get/Set the label text properties for each of the coordinate axes. Which
   * properties will be used depends on the selected Face being rendered.
   */
  void SetLabelTextProperty(int axis, vtkTextProperty*);
  vtkTextProperty* GetLabelTextProperty(int axis);
  //@}

  //@{
  /**
   * Set titles for each of the axes.
   */
  void SetTitle(int axis, const vtkStdString& title);
  const vtkStdString& GetTitle(int axis);
  //@}

  //@{
  /**
   * Get/set the numerical notation, standard, scientific or mixed (0, 1, 2).
   * Accepted values are vtkAxis::AUTO, vtkAxis::FIXED, vtkAxis::CUSTOM.
   */
  void SetNotation(int axis, int notation);
  int GetNotation(int axis);
  //@}

  //@{
  /**
   * Get/set the numerical precision to use, default is 2.
   */
  void SetPrecision(int axis, int val);
  int GetPrecision(int axis);
  //@}

  /**
   * Set custom tick positions for each of the axes.
   * The positions are deep copied. Set to NULL to not use custom tick positions
   * for the axis.
   */
  void SetCustomTickPositions(int axis, vtkDoubleArray* positions);

  // XXX GeohazardVis
  // Set custom axis labels in printf notation. The default is "%g"
  // If an empty string is passed, the axis notation style is set to vtkAxis::STANDARD_NOTATION;
  void SetPrintfAxisLabelFormat(int axis, const vtkStdString & formatString);
  // XXX End GeohazardVis

  //---------------------------------------------------------------------------
  // *** Properties to control grid rendering ***
  //---------------------------------------------------------------------------

  /**
   * Turn off to not generate polydata for the plane's grid.
   */
  void SetGenerateGrid(bool val) { this->PlaneActor->SetGenerateGrid(val); }
  bool GetGenerateGrid() { return this->PlaneActor->GetGenerateGrid(); }
  vtkBooleanMacro(GenerateGrid, bool);

  /**
   * Turn off to not generate the polydata for the plane's edges. Which edges
   * are rendered is defined by the EdgeMask.
   */
  void SetGenerateEdges(bool val) { this->PlaneActor->SetGenerateEdges(val); }
  bool GetGenerateEdges() { return this->PlaneActor->GetGenerateEdges(); }
  vtkBooleanMacro(GenerateEdges, bool);

  // Turn off to not generate the markers for the tick positions. Which egdes
  // are rendered is defined by the TickMask.
  void SetGenerateTicks(bool val) { this->PlaneActor->SetGenerateTicks(val); }
  bool GetGenerateTicks() { return this->PlaneActor->GetGenerateTicks(); }
  vtkBooleanMacro(GenerateTicks, bool);

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  //@{
  /**
   * Returns the prop bounds.
   */
  virtual double* GetBounds() VTK_OVERRIDE
  {
    this->GetGridBounds(this->Bounds);
    return this->Bounds;
  }
  //@}

  //@{
  /**
   * If true, the actor will always be rendered during the opaque pass.
   */
  vtkSetMacro(ForceOpaque, bool) vtkGetMacro(ForceOpaque, bool) vtkBooleanMacro(ForceOpaque, bool)
    //@}

    int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport* viewport) VTK_OVERRIDE;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  /**
   * Overridden to include the mtime for the text properties.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkGridAxes2DActor();
  ~vtkGridAxes2DActor();

  bool Update(vtkViewport* viewport);
  void UpdateTextProperties(vtkViewport* viewport);
  void UpdateLabelPositions(vtkViewport* viewport);
  void UpdateTextActors(vtkViewport* viewport);
  friend class vtkGridAxes3DActor;

  double GridBounds[6];
  int Face;
  unsigned int LabelMask;

  vtkTuple<vtkSmartPointer<vtkTextProperty>, 3> TitleTextProperty;
  vtkTuple<vtkSmartPointer<vtkTextProperty>, 3> LabelTextProperty;
  vtkTuple<vtkStdString, 3> Titles;

  vtkNew<vtkGridAxesHelper> Helper;
  vtkSmartPointer<vtkGridAxesPlane2DActor> PlaneActor;
  vtkNew<vtkAxis> AxisHelpers[3];
  vtkNew<vtkContextScene> AxisHelperScene;
  vtkTimeStamp UpdateLabelTextPropertiesMTime;

  bool ForceOpaque;

private:
  vtkGridAxes2DActor(const vtkGridAxes2DActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGridAxes2DActor&) VTK_DELETE_FUNCTION;

  class vtkLabels;
  vtkLabels* Labels;
  friend class vtkLabels;

  bool DoRender;
};

#endif
