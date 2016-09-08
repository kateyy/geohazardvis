#pragma once

#include <memory>

#include <QObject>

#include <vtkInteractorObserver.h>

#include <core/types.h>
#include <gui/gui_api.h>


class Highlighter;
class Picker;


class GUI_API PickerHighlighterInteractorObserver : public QObject, public vtkInteractorObserver
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

    bool picksOnMouseMove() const;
    void setPickOnMouseMove(bool doPick);

    const QString & pickedInfo() const;
    void requestPickedInfoUpdate();

signals:
    void pickedInfoChanged(const QString & infoText);
    void dataPicked(const VisualizationSelection & selection);
    void geometryChanged();

protected:
    PickerHighlighterInteractorObserver();
    ~PickerHighlighterInteractorObserver() override;

    void EventCallback(vtkObject * subject, unsigned long eventId, void * userData);

    void pick();
    void highlight();

private:
    unsigned long m_callbackTag;
    bool m_mouseMoved;
    bool m_pickOnMouseMove;

    std::unique_ptr<Picker> m_picker;
    std::unique_ptr<Highlighter> m_highlighter;

private:
    PickerHighlighterInteractorObserver(const PickerHighlighterInteractorObserver &) = delete;
    void operator=(const PickerHighlighterInteractorObserver &) = delete;
};
