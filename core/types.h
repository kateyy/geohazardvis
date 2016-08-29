#pragma once

#include <vector>

#include <vtkType.h>

#include <core/core_api.h>


class AbstractVisualizedData;
class DataObject;


enum class ContentType
{
    Rendered3D,
    Rendered2D,
    Context2D,
    invalid
};

/** Specify whether indices are related to points or to cells in the data set */
enum class IndexType
{
    points,
    cells,
    invalid
};


struct VisualizationSelection;


struct CORE_API DataSelection
{
    using Indices_t = std::vector<vtkIdType>;

    explicit DataSelection(DataObject * dataObject = nullptr);
    explicit DataSelection(
        DataObject * dataObject,
        IndexType indexType,
        Indices_t indices
    );
    explicit DataSelection(
        DataObject * dataObject,
        IndexType indexType,
        vtkIdType index
    );
    explicit DataSelection(const VisualizationSelection & visSelection);
    DataSelection & operator=(const VisualizationSelection & visSelection);

    ~DataSelection();

    /** Check whether the selection is completely empty, that is, it doesn't reference a
      * data object. However, it may still have an empty index list. */
    bool isEmpty() const;
    /** Check whether the index list is empty/invalid.
      * This returns true if the data object is unset (isEmpty()) or if the index list or index
      * type is empty or invalid. */
    bool isIndexListEmpty() const;

    bool operator==(const DataSelection & other) const;
    bool operator!=(const DataSelection & other) const;
    bool operator==(const VisualizationSelection & visSelection) const;
    bool operator!=(const VisualizationSelection & visSelection) const;

    void clear();

public:
    DataObject * dataObject;

    IndexType indexType;
    std::vector<vtkIdType> indices;
};

struct CORE_API VisualizationSelection
{
    using Indices_t = std::vector<vtkIdType>;

    explicit VisualizationSelection(AbstractVisualizedData * visualization = nullptr);
    explicit VisualizationSelection(
        AbstractVisualizedData * visualization,
        int visOutputPort,
        IndexType indexType,
        Indices_t indices = {}
    );
    explicit VisualizationSelection(
        AbstractVisualizedData * visualization,
        int visOutputPort,
        IndexType indexType,
        vtkIdType index
    );
    explicit VisualizationSelection(
        const DataSelection & dataSelection,
        AbstractVisualizedData * visualization,
        int visOutputPort
    );

    ~VisualizationSelection();

    /** Check whether the selection is completely empty, that is, it doesn't reference a
      * visualization. However, it may still have an empty index list. */
    bool isEmpty() const;
    /** Check whether the index list is empty/invalid.
    * This returns true if the visualization is unset (isEmpty()) or if the index list or index
    * type is empty or invalid. */
    bool isIndexListEmpty() const;

    bool operator==(const DataSelection & dataSelection) const;
    bool operator!=(const DataSelection & dataSelection) const;
    bool operator==(const VisualizationSelection & other) const;
    bool operator!=(const VisualizationSelection & other) const;

    void clear();

public:
    AbstractVisualizedData * visualization;
    int visOutputPort;

    IndexType indexType;
    Indices_t indices;
};
