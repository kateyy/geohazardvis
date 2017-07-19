/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/types.h>
#include <gui/gui_api.h>


class QTime;
class QTimer;
class vtkRenderer;

class AbstractVisualizedData;

class HighlighterImplementationSwitch;


class GUI_API Highlighter : public QObject
{
    Q_OBJECT

public:
    Highlighter();
    ~Highlighter() override;

    /** Clear the current highlighting target and set a new renderer */
    void setRenderer(vtkRenderer * renderer);
    vtkRenderer * renderer() const;

    /** Set visualization and a list of primitives to be highlighted. Discards previous indices. */
    void setTarget(const VisualizationSelection & selection);
    const VisualizationSelection & selection() const;

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
    void setTargetInternal(VisualizationSelection selection);

    void highlightPoints();
    void highlightCells();

    void checkDataVisibility();

private:
    vtkWeakPointer<vtkRenderer> m_renderer;
    VisualizationSelection m_selection;

    bool m_flashAfterSetTarget;
    unsigned int m_flashTimeMilliseconds;

    std::unique_ptr<HighlighterImplementationSwitch> m_impl;

    std::unique_ptr<QTimer> m_highlightFlashTimer;
    std::unique_ptr<QTime> m_highlightFlashTime;

private:
    Q_DISABLE_COPY(Highlighter)
};
