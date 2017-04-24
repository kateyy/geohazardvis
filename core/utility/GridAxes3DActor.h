#pragma once

#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>


/**
 * ParaView's vtkGridAxes3DActor adjusted to the needs of this project.
 *
 * - Label visibility can be toggled directly via the LabelsVisible property.
 * - Only one set of labels is shown per axis (x, y, and z)
 * - Grid line and border color can be set separately.
 */
class CORE_API GridAxes3DActor : public vtkGridAxes3DActor
{
public:
    vtkTypeMacro(GridAxes3DActor, vtkGridAxes3DActor);

    static GridAxes3DActor * New();

    vtkGetMacro(LabelsVisible, bool);
    void SetLabelsVisible(bool visible);
    vtkBooleanMacro(LabelsVisible, bool);

    /**
     * Set custom axis labels in printf notation. The default is "%g"
     * If an empty string is passed, the axis notation style is set to vtkAxis::STANDARD_NOTATION;
     */
    void SetPrintfAxisLabelFormat(int axis, const vtkStdString & formatString);

    /** Set colors for edges and grid lines. */
    void GetEdgeColor(unsigned char edgeColor[3]) const;
    unsigned char * GetEdgeColor() const;
    void SetEdgeColor(unsigned char edgeColor[3]);
    void SetEdgeColor(unsigned char r, unsigned char g, unsigned char b);
    void GetGridLineColor(unsigned char gridLineColor[3]) const;
    unsigned char * GetGridLineColor() const;
    void SetGridLineColor(unsigned char gridLineColor[3]);
    void SetGridLineColor(unsigned char r, unsigned char g, unsigned char b);

protected:
    GridAxes3DActor();
    ~GridAxes3DActor() override;

    void Update(vtkViewport * viewport) override;

private:
    /** Removed from the public interface. Visible axes and labels are handles by this class. */
    void SetLabelMask(unsigned int mask) override;

private:
    bool LabelsVisible;

private:
    GridAxes3DActor(const GridAxes3DActor &) = delete;
    void operator=(const GridAxes3DActor &) = delete;
};
