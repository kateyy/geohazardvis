#pragma once

#include <memory>

#include <QObject>

#include <vtkInteractorObserver.h>

#include <gui/gui_api.h>


class AbstractVisualizedData;
class Highlighter;
enum class IndexType;
class Picker;


class GUI_API PickerHighlighterInteractorObserver :public QObject, public vtkInteractorObserver
{
    Q_OBJECT

public:
    static PickerHighlighterInteractorObserver * New();
    vtkTypeMacro(PickerHighlighterInteractorObserver, vtkInteractorObserver);

    void SetEnabled(int enabling) override;
    void SetInteractor(vtkRenderWindowInteractor * interactor) override;

    Picker & picker();
    const Picker & picker() const;
    Highlighter & highlighter();
    const Highlighter & highlighter() const;

signals:
    void pickedInfoChanged(const QString & infoText);
    void dataPicked(AbstractVisualizedData * visualizedData, vtkIdType index, IndexType indexType);

protected:
    PickerHighlighterInteractorObserver();
    ~PickerHighlighterInteractorObserver() override;

    void EventCallback(vtkObject * subject, unsigned long eventId, void * userData);

    void pickHighlight();

private:
    unsigned long m_callbackTag;
    bool m_mouseMoved;

    std::unique_ptr<Picker> m_picker;
    std::unique_ptr<Highlighter> m_highlighter;

private:
    PickerHighlighterInteractorObserver(const PickerHighlighterInteractorObserver &) = delete;
    void operator=(const PickerHighlighterInteractorObserver &) = delete;
};
