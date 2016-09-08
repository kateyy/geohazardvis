#pragma once

#include <memory>

#include <gui/gui_api.h>


class QString;
class vtkDataArray;
class vtkRenderer;
class vtkVector2i;

class Picker_private;
struct VisualizationSelection;


class GUI_API Picker
{
public:
    Picker();
    virtual ~Picker();

    void pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer);

    const QString & pickedObjectInfoString() const;

    const VisualizationSelection & pickedObjectInfo() const;

    vtkDataArray * pickedScalarArray();

    Picker(Picker && other);

private:
    std::unique_ptr<Picker_private> d_ptr;

private:
    Picker(const Picker &) = delete;
    void operator=(const Picker &) = delete;
};
