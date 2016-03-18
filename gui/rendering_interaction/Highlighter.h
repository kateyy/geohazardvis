#pragma once

#include <memory>

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <gui/gui_api.h>


class QTime;
class QTimer;
class vtkIdTypeArray;
class vtkRenderer;

class AbstractVisualizedData;
enum class IndexType;

class HighlighterImplementationSwitch;


class GUI_API Highlighter : public QObject
{
    Q_OBJECT

public:
    Highlighter();
    virtual ~Highlighter();

    /** Clear the current highlighting target and set a new renderer */
    void setRenderer(vtkRenderer * renderer);
    vtkRenderer * renderer() const;

    /** Set visualization and one primitive on it to be highlighted. Discards previous indices. */
    void setTarget(AbstractVisualizedData * vis, int visOutputPort, vtkIdType index, IndexType indexType);
    /** Set visualization and a list of primitives to be highlighted. Discards previous indices.
      * @param indices Cannot be const, due to the non-const vtkIdTypeArray getters. */
    void setTarget(AbstractVisualizedData * vis, int visOutputPort, vtkIdTypeArray & indices, IndexType indexType);
    AbstractVisualizedData * targetVisualization() const;
    vtkIdTypeArray * targetIndices();
    vtkIdType lastTargetIndex() const;
    IndexType targetIndexType() const;

    /** Remove all highlights from the renderer, clear all internal settings */
    void clear();
    /** Hide highlighting primitives and reset the internal index list
      * Remember the current renderer view, data object and index type */
    void clearIndices();

    void flashTargets();

    void setFlashAfterTarget(bool doFlash);
    bool flashAfterSetTarget() const;

    void setFlashTimeMilliseconds(unsigned int milliseconds);
    unsigned int flashTimeMilliseconds() const;

signals:
    void geometryChanged();

private:
    void updateHighlight();
    void setTargetInternal(AbstractVisualizedData * vis, int visOutputPort, IndexType indexType);

    void highlightPoints();
    void highlightCells();

    void checkDataVisibility();

private:
    vtkWeakPointer<vtkRenderer> m_renderer;
    AbstractVisualizedData * m_visualizedData;
    int m_visOutputPort;
    vtkSmartPointer<vtkIdTypeArray> m_indices;
    IndexType m_indexType;

    bool m_flashAfterSetTarget;
    unsigned int m_flashTimeMilliseconds;

    std::unique_ptr<HighlighterImplementationSwitch> m_impl;

    std::unique_ptr<QTimer> m_highlightFlashTimer;
    std::unique_ptr<QTime> m_highlightFlashTime;
};
