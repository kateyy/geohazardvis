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
#include <vector>

#include <QList>
#include <QMap>
#include <QSet>

#include <gui/data_view/AbstractRenderView.h>


class RendererImplementationSwitch;


class GUI_API RenderView : public AbstractRenderView
{
public:
    RenderView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    ContentType contentType() const override;

    void lookAtData(const DataSelection & selection, int subViewIndex = -1) override;
    void lookAtData(const VisualizationSelection & selection, int subViewIndex = -1) override;

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const override;
    int subViewContaining(const AbstractVisualizedData & visualizedData) const override;

    bool isEmpty() const override;

    // remove from public interface as soon as possible
    RendererImplementation & implementation() const override;

protected:
    void closeEvent(QCloseEvent * event) override;
    
    void initializeRenderContext() override;

    void visualizationSelectionChangedEvent(const VisualizationSelection & selection) override;

    void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) override;
    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int subViewIndex) override;
    QList<DataObject *> dataObjectsImpl(int subViewIndex) const override;
    void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) override;

    QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const override;

private:
    void updateImplementation(const QList<DataObject *> & contents);

    // data handling

    AbstractVisualizedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    // remove some data objects from internal lists
    // @return list of dangling rendered data object that you have to delete.
    std::vector<std::unique_ptr<AbstractVisualizedData>> removeFromInternalLists(QList<DataObject *> dataObjects = {});

    /** update configuration widgets to focus on my content. */
    void updateGuiForSelectedData(AbstractVisualizedData * content);
    void updateGuiForRemovedData();

private:
    std::unique_ptr<RendererImplementationSwitch> m_implementationSwitch;
    bool m_closingRequested;

    // rendered representations of data objects for this view
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_contents;
    // objects that were loaded to the GPU but are currently not rendered 
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_contentCache;
    QMap<DataObject *, AbstractVisualizedData *> m_dataObjectToVisualization;
    // DataObjects, that emitted deleted() and that we didn't remove yet
    QSet<DataObject *> m_deletedData;

private:
    Q_DISABLE_COPY(RenderView)
};
