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

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <QObject>
#include <QStringList>

#include <core/types.h>
#include <gui/gui_api.h>
#include <gui/data_view/t_QVTKWidgetFwd.h>


class vtkIdTypeArray;
class vtkRenderWindowInteractor;

class AbstractRenderView;
class AbstractVisualizedData;
enum class ContentType;
enum class IndexType;
class DataObject;
class DataMapping;
struct CoordinateSystemSpecification;


class GUI_API RendererImplementation : public QObject
{
    Q_OBJECT

public:
    explicit RendererImplementation(AbstractRenderView & renderView);
    ~RendererImplementation() override;

    AbstractRenderView & renderView() const;
    DataMapping & dataMapping() const;

    virtual QString name() const = 0;

    virtual ContentType contentType() const = 0;

    virtual void applyCurrentCoordinateSystem(const CoordinateSystemSpecification & spec);

    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) = 0;

    const QStringList & supportedInteractionStrategies() const;
    void setInteractionStrategy(const QString & strategyName);
    const QString & currentInteractionStrategy() const;

    virtual void activate(t_QVTKWidget & qvtkWidget);
    virtual void deactivate(t_QVTKWidget & qvtkWidget);

    virtual void render() = 0;

    virtual vtkRenderWindowInteractor * interactor() = 0;

    virtual unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const;

    /** add newly created rendered data
        actual data visibility depends on the object's configuration */
    void addContent(AbstractVisualizedData * content, unsigned int subViewIndex);
    /** remove all references to the object and its contents */
    void removeContent(AbstractVisualizedData * content, unsigned int subViewIndex);
    /** This is called once after a set of data objects was removed or added to the render view */
    void renderViewContentsChanged();

    /** mark dataObject (and, if set, it's point/cell) as current selection */
    void setSelection(const VisualizationSelection & selection);
    void clearSelection();
    const VisualizationSelection & selection() const;
    /** Move camera so that the specified part of the visualization becomes visible. */
    virtual void lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex) = 0;

    /** Resets the camera so that all view content are visible.
        Moves back to the initial position if requested. */
    virtual void resetCamera(bool toInitialPosition, unsigned int subViewIndex) = 0;

    virtual void setAxesVisibility(bool visible) = 0;

    virtual bool canApplyTo(const QList<DataObject *> & data) = 0;

    virtual std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject & dataObject) const = 0;

    using ImplementationConstructor = std::function<std::unique_ptr<RendererImplementation>(AbstractRenderView & view)>;
    static const std::vector<ImplementationConstructor> & constructors();

signals:
    void interactionStrategyChanged(const QString & strategyName);
    void supportedInteractionStrategiesChanged(const QStringList & stategyNames);

protected:

    /** Override to add visual contents to the view.
        In the overrider, call addConnectionForContent for each Qt signal/slot 
        connection related to this content */
    virtual void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) = 0;
    virtual void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) = 0;
    virtual void onRenderViewContentsChanged();

    virtual void onSetSelection(const VisualizationSelection & selection) = 0;
    virtual void onClearSelection() = 0;

    /** Call in sub-classes, to update the list of supported interaction strategies
    * @param strategyName Names of supported strategies. These names must be unique. 
    * @param currentStrategy Set the strategy that will be used now. Must be contained in strategyNames. */
    void setSupportedInteractionStrategies(const QStringList & strategyNames, const QString & currentStrategy);
    /** Subclasses should implement this, if they support multiple interaction styles.
    * Implement the actual switch to the new strategy here. */
    virtual void updateForCurrentInteractionStrategy(const QString & strategyName);

    void addConnectionForContent(AbstractVisualizedData * content,
        const QMetaObject::Connection & connection);

    template <typename ImplType> static bool registerImplementation();

protected:
    AbstractRenderView & m_renderView;

private:
    static std::vector<ImplementationConstructor> & s_constructors();

    std::map<AbstractVisualizedData *, std::vector<QMetaObject::Connection>> m_visConnections;

    VisualizationSelection m_selection;

    QStringList m_supportedInteractionStrategies;
    QString m_currentInteractionStrategy;

private:
    Q_DISABLE_COPY(RendererImplementation)
};

template <typename ImplType>
bool RendererImplementation::registerImplementation()
{
    s_constructors().emplace_back(
        [](AbstractRenderView & view) { return std::make_unique<ImplType>(view); }
    );

    return true;
}
