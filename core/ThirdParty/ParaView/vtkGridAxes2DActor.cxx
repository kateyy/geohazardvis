/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxes2DActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridAxes2DActor.h"

#include "vtkAxis.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <vector>

namespace
{
  // The point is assumed to be in Viewport coordinate system i.e X,Y pixels in
  // the viewport.
  template <class T>
  inline bool IsInViewport(vtkViewport* viewport, const vtkVector2<T>& point)
    {
    vtkVector2d npos(point.GetX(), point.GetY());
    viewport->ViewportToNormalizedViewport(npos[0], npos[1]);
    return (npos[0] >= 0.0 && npos[0] <= 1.0 && npos[1] >= 0.0 && npos[1] <= 1.0);
    }
}

class vtkGridAxes2DActor::vtkLabels
{
  static inline bool RenderTextActor(vtkTextActor* actor,  bool do_background, bool do_foreground)
    {
    int textDisplayLocation = actor->GetProperty()->GetDisplayLocation();
    return (textDisplayLocation == VTK_FOREGROUND_LOCATION && do_foreground) ||
      (textDisplayLocation == VTK_BACKGROUND_LOCATION && do_background);
    }
public:
  typedef std::vector<vtkSmartPointer<vtkTextActor> > TickLabelsType;
  TickLabelsType TickLabels[4];
  vtkNew<vtkTextActor> TitleLabels[4];
  vtkVector2i Justifications[4];

  vtkLabels()
    {
    for (int cc=0; cc < 4; cc++)
      {
      this->TitleLabels[cc]->SetVisibility(0);
      }
    }
  static void ResizeLabels(TickLabelsType& labels, size_t new_size, vtkTextProperty* property=NULL)
    {
    labels.resize(new_size);
    for (TickLabelsType::iterator iter=labels.begin(); iter != labels.end(); ++iter)
      {
      if (!iter->GetPointer())
        {
        (*iter) = vtkSmartPointer<vtkTextActor>::New();
        if (property)
          {
          iter->GetPointer()->GetTextProperty()->ShallowCopy(property);
          }
        }
      }
    }

  int RenderOpaqueGeometry(vtkViewport* viewport, bool do_background, bool do_foreground)
    {
    int counter = 0;
    for (int cc=0; cc < 4; cc++)
      {
      for (TickLabelsType::iterator iter=this->TickLabels[cc].begin();
        iter != this->TickLabels[cc].end(); ++iter)
        {
        if (this->RenderTextActor(iter->GetPointer(), do_background, do_foreground))
          {
          counter += iter->GetPointer()->RenderOpaqueGeometry(viewport);
          }
        }
      if (this->TitleLabels[cc]->GetVisibility() &&
        this->RenderTextActor(this->TitleLabels[cc].Get(), do_background, do_foreground))
        {
        counter += this->TitleLabels[cc]->RenderOpaqueGeometry(viewport);
        }
      }
    return counter;
    }
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport, bool do_background, bool do_foreground)
    {
    int counter = 0;
    for (int cc=0; cc < 4; cc++)
      {
      for (TickLabelsType::iterator iter=this->TickLabels[cc].begin();
        iter != this->TickLabels[cc].end(); ++iter)
        {
        if (this->RenderTextActor(iter->GetPointer(), do_background, do_foreground))
          {
          counter += iter->GetPointer()->RenderTranslucentPolygonalGeometry(viewport);
          }
        }
      if (this->TitleLabels[cc]->GetVisibility() &&
        this->RenderTextActor(this->TitleLabels[cc].Get(), do_background, do_foreground))
        {
        counter += this->TitleLabels[cc]->RenderTranslucentPolygonalGeometry(viewport);
        }
      }
    return counter;
    }
  int RenderOverlay(vtkViewport* viewport, bool do_background, bool do_foreground)
    {
    int counter = 0;
    for (int cc=0; cc < 4; cc++)
      {
      for (TickLabelsType::iterator iter=this->TickLabels[cc].begin();
        iter != this->TickLabels[cc].end(); ++iter)
        {
        if (this->RenderTextActor(iter->GetPointer(), do_background, do_foreground))
          {
          counter += iter->GetPointer()->RenderOverlay(viewport);
          }
        }
      if (this->TitleLabels[cc]->GetVisibility() &&
        this->RenderTextActor(this->TitleLabels[cc].Get(), do_background, do_foreground))
        {
        counter += this->TitleLabels[cc]->RenderOverlay(viewport);
        }
      }
    return counter;
    }
  void ReleaseGraphicsResources(vtkWindow* win)
    {
    for (int cc=0; cc < 4; cc++)
      {
      for (TickLabelsType::iterator iter=this->TickLabels[cc].begin();
        iter != this->TickLabels[cc].end(); ++iter)
        {
        iter->GetPointer()->ReleaseGraphicsResources(win);
        }
      this->TitleLabels[cc]->ReleaseGraphicsResources(win);
      }
    }
};

vtkStandardNewMacro(vtkGridAxes2DActor);
//----------------------------------------------------------------------------
vtkGridAxes2DActor::vtkGridAxes2DActor() :
  Face(vtkGridAxes2DActor::MIN_YZ),
  LabelMask(0xFF),
  EnableLayerSupport(false),
  BackgroundLayer(0),
  ForegroundLayer(0),
  GeometryLayer(0),
  Labels(new vtkGridAxes2DActor::vtkLabels()),
  DoRender(false)
{
  this->PlaneActor.TakeReference(vtkGridAxesPlane2DActor::New(this->Helper.Get()));
  for (int cc=0; cc < 3; cc++)
    {
    this->AxisHelpers[cc]->SetScene(this->AxisHelperScene.GetPointer());
    this->AxisHelpers[cc]->SetPosition(vtkAxis::LEFT);
    this->AxisHelpers[cc]->SetBehavior(vtkAxis::FIXED);
    this->TitleTextProperty[cc] = vtkSmartPointer<vtkTextProperty>::New();
    this->LabelTextProperty[cc] = vtkSmartPointer<vtkTextProperty>::New();
    }
}

//----------------------------------------------------------------------------
vtkGridAxes2DActor::~vtkGridAxes2DActor()
{
  delete this->Labels;
  this->Labels = NULL;
}

//----------------------------------------------------------------------------
unsigned long vtkGridAxes2DActor::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  for (int cc=0; cc < 3; cc++)
    {
    mtime = std::max(mtime, this->LabelTextProperty[cc]->GetMTime());
    mtime = std::max(mtime, this->TitleTextProperty[cc]->GetMTime());
    }

  return mtime;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetTitle(int axis, const vtkStdString& title)
{
  if (axis >=0 && axis < 3 && this->Titles[axis] != title)
    {
    this->Titles[axis] = title;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const vtkStdString& vtkGridAxes2DActor::GetTitle(int axis)
{
  static vtkStdString nullstring;
  return (axis >=0 && axis < 3)? this->Titles[axis] : nullstring;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetNotation(int axis, int notation)
{
  if (axis >=0 && axis < 3 && this->AxisHelpers[axis]->GetNotation() != notation)
    {
    this->AxisHelpers[axis]->SetNotation(notation);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::GetNotation(int axis)
{
  return (axis >=0 && axis < 3)? this->AxisHelpers[axis]->GetNotation() : vtkAxis::AUTO;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetPrecision(int axis, int precision)
{
  if (axis >=0 && axis < 3 && this->AxisHelpers[axis]->GetPrecision() != precision)
    {
    this->AxisHelpers[axis]->SetPrecision(precision);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::GetPrecision(int axis)
{
  return (axis >=0 && axis < 3)? this->AxisHelpers[axis]->GetPrecision() : vtkAxis::AUTO;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetProperty(vtkProperty* property)
{
  this->PlaneActor->SetProperty(property);
}

//----------------------------------------------------------------------------
vtkProperty* vtkGridAxes2DActor::GetProperty()
{
  return this->PlaneActor->GetProperty();
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetTitleTextProperty(int axis, vtkTextProperty* tprop)
{
  if (axis >=0 && axis < 3 && this->TitleTextProperty[axis] != tprop && tprop != NULL)
    {
    this->TitleTextProperty[axis] = tprop;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxes2DActor::GetTitleTextProperty(int axis)
{
  return (axis >=0 && axis < 3)? this->TitleTextProperty[axis] : NULL;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetLabelTextProperty(int axis, vtkTextProperty* tprop)
{
  if (axis >=0 && axis < 3 && this->LabelTextProperty[axis] != tprop && tprop != NULL)
    {
    this->LabelTextProperty[axis] = tprop;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxes2DActor::GetLabelTextProperty(int axis)
{
  return (axis >=0 && axis < 3)? this->LabelTextProperty[axis] : NULL;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::SetCustomTickPositions(int axis, vtkDoubleArray* positions)
{
  if (axis >= 0 && axis < 3)
    {
    this->AxisHelpers[axis]->SetCustomTickPositions(positions);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->Update(viewport);
  if (!this->DoRender)
    {
    return 0;
    }

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport);
  assert(renderer != NULL);
  bool do_background = (this->EnableLayerSupport == false || renderer->GetLayer() == this->BackgroundLayer);
  bool do_foreground = (this->EnableLayerSupport == false || renderer->GetLayer() == this->ForegroundLayer);

  if (do_background)
    {
    this->UpdateTextActors(viewport);
    }

  int counter = 0;
  counter += this->Labels->RenderOpaqueGeometry(viewport, do_background, do_foreground);
  counter += this->PlaneActor->RenderOpaqueGeometry(viewport);
  return counter;
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  if (!this->DoRender)
    {
    return 0;
    }

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport);
  assert(renderer != NULL);
  bool do_background = (this->EnableLayerSupport == false || renderer->GetLayer() == this->BackgroundLayer);
  bool do_foreground = (this->EnableLayerSupport == false || renderer->GetLayer() == this->ForegroundLayer);

  int counter = 0;
  counter += this->Labels->RenderTranslucentPolygonalGeometry(viewport, do_background, do_foreground);
  counter += this->PlaneActor->RenderTranslucentPolygonalGeometry(viewport);
  return counter;
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::RenderOverlay(vtkViewport* viewport)
{
  if (!this->DoRender)
    {
    return 0;
    }

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport);
  assert(renderer != NULL);
  bool do_background = (this->EnableLayerSupport == false || renderer->GetLayer() == this->BackgroundLayer);
  bool do_foreground = (this->EnableLayerSupport == false || renderer->GetLayer() == this->ForegroundLayer);

  int counter = 0;
  counter += this->Labels->RenderOverlay(viewport, do_background, do_foreground);
  counter += this->PlaneActor->RenderOverlay(viewport);
  return counter;
}

//----------------------------------------------------------------------------
int vtkGridAxes2DActor::HasTranslucentPolygonalGeometry()
{
  return this->PlaneActor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Labels->ReleaseGraphicsResources(win);
  this->PlaneActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
bool vtkGridAxes2DActor::Update(vtkViewport* viewport)
{
  this->Helper->SetGridBounds(this->GridBounds);
  this->Helper->SetFace(this->Face);
  this->Helper->SetMatrix(this->GetMatrix());
  this->Helper->SetLabelMask(this->LabelMask);
  this->PlaneActor->SetUserMatrix(this->GetMatrix());
  this->PlaneActor->SetEnableLayerSupport(this->EnableLayerSupport);
  this->PlaneActor->SetLayer(this->GeometryLayer);

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport);
  assert(renderer != NULL);

  // This is needed so the vtkAxis labels account for tile scaling.
  this->AxisHelperScene->SetRenderer(renderer);

  if (this->EnableLayerSupport == false || renderer->GetLayer() == this->BackgroundLayer)
    {
    if (
      (this->Helper->UpdateForViewport(viewport) == false) ||
      (this->GetProperty()->GetBackfaceCulling() && this->Helper->GetBackface()) ||
      (this->GetProperty()->GetFrontfaceCulling() && !this->Helper->GetBackface()))
      {
      this->DoRender = false;
      return false;
      }
    this->DoRender = true;
    this->UpdateTextProperties(viewport);
    this->UpdateLabelPositions(viewport);
    }

  return this->DoRender;
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::UpdateTextProperties(vtkViewport*)
{
  // Update text properties.
  if (this->GetMTime() < this->UpdateLabelTextPropertiesMTime)
    {
    return;
    }
  for (int cc=0; cc < 3; cc++)
    {
    // Pass the current text properties to the vtkAxis
    // objects so they can place the labels appropriately using the current
    // label text properties.
    this->AxisHelpers[cc]->GetLabelProperties()->ShallowCopy(this->LabelTextProperty[cc]);
    }

  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  for (int cc=0; cc < 4; cc++)
    {
    this->Labels->TitleLabels[cc]->GetTextProperty()->ShallowCopy(
      this->TitleTextProperty[activeAxes[cc%2]]);
    for (vtkLabels::TickLabelsType::iterator iter = this->Labels->TickLabels[cc].begin();
      iter != this->Labels->TickLabels[cc].end(); ++iter)
      {
      iter->GetPointer()->GetTextProperty()->ShallowCopy(
        this->LabelTextProperty[activeAxes[cc%2]].Get());
      }
    }

  this->UpdateLabelTextPropertiesMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::UpdateLabelPositions(vtkViewport*)
{
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<vtkVector2d, 4> axisVectors = this->Helper->GetViewportVectors();
  const vtkTuple<vtkVector2d, 4> axisNormals = this->Helper->GetViewportNormals();
  const vtkTuple<bool, 4>& labelVisibilties = this->Helper->GetLabelVisibilities();

  vtkAxis* activeAxisHelpers[2];
  activeAxisHelpers[0] = this->AxisHelpers[activeAxes[0]].GetPointer();
  activeAxisHelpers[1] = this->AxisHelpers[activeAxes[1]].GetPointer();

  // Determine the number of labels to place and the text to use for them.
  for (int cc=0; cc <2; cc++)
    {
    if (std::abs(axisVectors[cc].GetX()) > std::abs(axisVectors[cc].GetY()))
      {
      // normal is more horizontal hence axis is more vertical.
      activeAxisHelpers[cc]->SetPoint1(0, 0);
      activeAxisHelpers[cc]->SetPoint2(std::abs(axisVectors[cc].GetX()), 0);
      activeAxisHelpers[cc]->SetPosition(vtkAxis::BOTTOM);
      }
    else
      {
      activeAxisHelpers[cc]->SetPoint1(0, 0);
      activeAxisHelpers[cc]->SetPoint2(0, std::abs(axisVectors[cc].GetY()));
      activeAxisHelpers[cc]->SetPosition(vtkAxis::LEFT);
      }
    }
  activeAxisHelpers[0]->SetUnscaledRange(
    this->GridBounds[2*activeAxes.GetX()],
    this->GridBounds[2*activeAxes.GetX()+1]);
  activeAxisHelpers[1]->SetUnscaledRange(
    this->GridBounds[2*activeAxes.GetY()],
    this->GridBounds[2*activeAxes.GetY()+1]);

  activeAxisHelpers[0]->Update();
  activeAxisHelpers[1]->Update();

  // Tell the plane actor where we've decided to place the labels.
  this->PlaneActor->SetTickPositions(activeAxes[0], activeAxisHelpers[0]->GetTickPositions());
  this->PlaneActor->SetTickPositions(activeAxes[1], activeAxisHelpers[1]->GetTickPositions());

  //----------------------------------------------------------------------------------------
  // Now compute label justifications to control their placement.
  vtkVector2d xaxis(1, 0);
  vtkVector2d yaxis(0, 1);
  for (int cc=0; cc < 4; cc++)
    {
    this->Labels->Justifications[cc].SetX(VTK_TEXT_CENTERED);
    this->Labels->Justifications[cc].SetY(VTK_TEXT_CENTERED);
    if (!labelVisibilties[cc])
      {
      continue;
      }

    const vtkVector2d& axisNormal = axisNormals[cc];
    double cosTheta = axisNormal.Dot(xaxis);
    double sinTheta = axisNormal.Dot(yaxis);

    if (std::abs(axisVectors[cc].GetX()) > std::abs(axisVectors[cc].GetY()))
      {
      // horizontal axis.

      // Sin() will be +'ve for labels on top, -'ve for labels on bottom of
      // axis. Thus vertical justification will be bottom and top respectively.
      this->Labels->Justifications[cc].SetY(sinTheta >=0?
        VTK_TEXT_BOTTOM : VTK_TEXT_TOP);

      if (std::abs(cosTheta) < 0.342020143) //math.sin(math.radians(20))
        {
        // very vertical.
        this->Labels->Justifications[cc].SetX(VTK_TEXT_CENTERED);
        }
      else
        {
        // Cos() +'ve ==> labels right of axis and -'ve for labels on left i.e.
        // with horizontal justification set to left and right respectively.
        this->Labels->Justifications[cc].SetX(cosTheta >=0?
          VTK_TEXT_LEFT : VTK_TEXT_RIGHT);
        }
      }
    else
      {
      // vertical axis.

      // Cos() will be +'ve for labels on right of axis, while -ve for labels on left of axis.
      this->Labels->Justifications[cc].SetX(cosTheta >=0?
        VTK_TEXT_LEFT : // anchor left i.e. label on right
        VTK_TEXT_RIGHT); // anchor right

      if (std::abs(sinTheta) < 0.342020143) // math.sin(math.radians(20))
        {
        this->Labels->Justifications[cc].SetY(VTK_TEXT_CENTERED);
        }
      else
        {
        // Sin() +'ve => labels on top of axis, -'ve labels on bottom of axis.
        this->Labels->Justifications[cc].SetY(
          sinTheta >=0?
          VTK_TEXT_BOTTOM : // anchor bottom i.e. labels on top
          VTK_TEXT_TOP); // anchor top.
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::UpdateTextActors(vtkViewport* viewport)
{
  // Decide on text rendering layer (back or front).
  int textDisplayLocation = this->Helper->GetBackface()?
    VTK_BACKGROUND_LOCATION : VTK_FOREGROUND_LOCATION;

  const vtkTuple<vtkVector3d, 4>& gridPoints = this->Helper->GetPoints();
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<bool, 4>& labelVisibilties = this->Helper->GetLabelVisibilities();
  const vtkTuple<vtkVector2d, 4>& axisNormals = this->Helper->GetViewportNormals();
  const vtkTuple<vtkVector2d, 4>& viewportPoints = this->Helper->GetViewportPointsAsDouble();

  vtkAxis* activeAxisHelpers[2];
  activeAxisHelpers[0] = this->AxisHelpers[activeAxes[0]].GetPointer();
  activeAxisHelpers[1] = this->AxisHelpers[activeAxes[1]].GetPointer();

  for (int index=0; index < 4; index++)
    {
    int axis = index % 2;
    vtkStringArray* labels = activeAxisHelpers[axis]->GetTickLabels();
    vtkDoubleArray* tickPositions = activeAxisHelpers[axis]->GetTickPositions();
    vtkIdType numTicks = labelVisibilties[index]? tickPositions->GetNumberOfTuples() : 0;
    if (numTicks == 0)
      {
      vtkLabels::ResizeLabels(this->Labels->TickLabels[index], 0);
      continue;
      }

    typedef std::pair<vtkVector2i, vtkIdType> LabelPositionsPair;
    typedef std::vector<LabelPositionsPair> LabelPositionsPairVector;
    LabelPositionsPairVector labelPositions;

    vtkNew<vtkCoordinate> coordinate;
    coordinate->SetCoordinateSystemToWorld();

    /// XXX: improve this.
    vtkVector2i offset(
      vtkContext2D::FloatToInt(axisNormals[index].GetX()*10),
      vtkContext2D::FloatToInt(axisNormals[index].GetY()*10));

    for (vtkIdType cc=0; cc < numTicks; ++cc)
      {
      vtkVector3d tickPosition = gridPoints[index];
      tickPosition[activeAxes[axis]] = tickPositions->GetValue(cc);

      vtkVector3d transformedPosition = this->Helper->TransformPoint(tickPosition);
      coordinate->SetValue(transformedPosition.GetData());
      vtkVector2i pos(coordinate->GetComputedViewportValue(viewport));
      if (IsInViewport(viewport, pos))
        {
        // Need to offset the position in viewport space along the axis normal
        // (which too is in viewport space!!).
        pos = pos + offset;
        labelPositions.push_back(LabelPositionsPair(pos, cc));
        }
      }

    vtkLabels::ResizeLabels(this->Labels->TickLabels[index],
      static_cast<vtkIdType>(labelPositions.size()),
      activeAxisHelpers[axis]->GetLabelProperties());
    vtkIdType actorIndex = 0;
    for (LabelPositionsPairVector::const_iterator iter = labelPositions.begin();
      iter != labelPositions.end(); ++ iter, ++actorIndex)
      {
      vtkIdType cc = iter->second;
      vtkTextActor* labelActor = this->Labels->TickLabels[index][actorIndex];
      labelActor->SetInput(labels->GetValue(cc).c_str());
      labelActor->GetProperty()->SetDisplayLocation(textDisplayLocation);

      // pass justification information.
      labelActor->GetTextProperty()->SetJustification(this->Labels->Justifications[index].GetX());
      labelActor->GetTextProperty()->SetVerticalJustification(this->Labels->Justifications[index].GetY());

      // This is consistent with what vtkAxis which is not the same as the
      // vtkTextRepresentation or vtkScalarBarActor.
      labelActor->SetTextScaleModeToNone();
      labelActor->ComputeScaledFont(viewport);
      labelActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
      labelActor->GetPositionCoordinate()->SetValue(iter->first.GetX(), iter->first.GetY());
      }
    }

  for (int index=0; index < 4; index++)
    {
    // Setup title text.
    vtkTextActor* titleActor = this->Labels->TitleLabels[index].GetPointer();
    const vtkStdString& label = this->Titles[activeAxes[index%2]];
    if (label.empty() == false && labelVisibilties[index])
      {
      vtkVector2d midPoint = (viewportPoints[index] + viewportPoints[(index+1) % 4])*0.5;
      /// XXX: improve this.
      vtkVector2i offset(
        vtkContext2D::FloatToInt(axisNormals[index].GetX()*30),
        vtkContext2D::FloatToInt(axisNormals[index].GetY()*30));
      midPoint = midPoint + vtkVector2d(offset.GetX(), offset.GetY());
      if (IsInViewport(viewport, midPoint))
        {
        titleActor->SetInput(label.c_str());
        titleActor->GetTextProperty()->SetJustification(
          this->Labels->Justifications[index].GetX());
        titleActor->GetTextProperty()->SetVerticalJustification(
          this->Labels->Justifications[index].GetY());
        titleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
        titleActor->GetPositionCoordinate()->SetValue(midPoint.GetX(), midPoint.GetY());
        titleActor->SetTextScaleModeToNone();
        titleActor->ComputeScaledFont(viewport);
        titleActor->SetVisibility(1);
        titleActor->GetProperty()->SetDisplayLocation(textDisplayLocation);
        }
      else
        {
        titleActor->SetVisibility(0);
        }
      }
    else
      {
      titleActor->SetVisibility(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkGridAxes2DActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
