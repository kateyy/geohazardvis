#pragma once

#include <memory>

#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkAlgorithm;
class vtkInformationStringKey;
class vtkPolyData;
class DataObject;


class CORE_API DeformationTimeSeriesTextFileReader
{
public:
    /**
     * Reference the original string representation of a timestamp.
     * VTK stores time steps as double, which might not result in the same representation the user
     * would expect.
     */
    static vtkInformationStringKey * TIME_STEP_STRING();

public:
    explicit DeformationTimeSeriesTextFileReader(const QString & fileName = {});
    ~DeformationTimeSeriesTextFileReader();

    void setFileName(const QString & fileName);
    const QString & fileName() const;

    enum State
    {
        notRead = 0x0,
        validInformation = 0x1,
        validData = 0x2,
        invalidFileName = 0x4,
        invalidFileFormat = 0x8,
        missingData = 0x10,
    };

    State state() const;

    enum class Coordinate
    {
        UTM_WGS84,
        AzimuthRange,
        LongitudeLatitude,
    };
    /*
     * Set the coordinates that will be used to define the geometry of the output data set.
     * This is UTM_WGS84 by default.
     * Other available coordinates will be appended as point data to the data set.
     * If the selected coordinate type is not stored in the data set, State::missing data will be
     * set on this->state()
     */
    void setCoordinatesToUse(Coordinate coordinate);
    Coordinate coordinateToUse() const;

    /**
     * Read the whole file.
     * This first calls readInformation if necessary and than reads the actual data.
     */
    State readData();
    /** Read the header of the file */
    State readInformation();

    /**
     * Generate and return on instance of the best matching DataObject containing the read data.
     * It is required to successfully call readFile() before.
     * That is, this->state() has to have State::validData set.
     * @return a valid DataObject instance if valid data was read before, or an empty pointer
     * otherwise.
     */
    std::unique_ptr<DataObject> generateDataObject();

    /** Reset configurations, information and discard read data. */
    void clear();

    /** @return Number of employed dates */
    int numberOfDates() const;

    /** @return Deformation measurement unit */
    const QString & deformationUnitString();

    static const char * arrayName_Coords(Coordinate coordType);
    static const char * arrayName_UTM_WGS84();
    static const char * arrayName_AzimuthRange();
    static const char * arrayName_LongitudeLatitude();
    static const char * arrayName_TemporalInterferometricCoherence();
    static const char * arrayName_DeformationVelocity();
    static const char * arrayName_ResidualTopography();
    static const char * arrayName_DeformationTimeSeries();

    DeformationTimeSeriesTextFileReader(const DeformationTimeSeriesTextFileReader &) = delete;
    void operator=(const DeformationTimeSeriesTextFileReader &) = delete;
    DeformationTimeSeriesTextFileReader(DeformationTimeSeriesTextFileReader && other);
    DeformationTimeSeriesTextFileReader & operator=(DeformationTimeSeriesTextFileReader && other);

private:
    State setState(State state);
    void clearData();

private:
    State m_state;

    QString m_fileName;
    Coordinate m_coordinatesToUse;

    uint64_t m_dataOffset;
    int m_numColumnsBeforeDeformations;
    int m_numDates;
    QString m_deformationUnitString;

    vtkSmartPointer<vtkPolyData> m_readPolyData;
    vtkSmartPointer<vtkAlgorithm> m_temporalDataSource;
};
