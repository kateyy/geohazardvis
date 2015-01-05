#include "RenderConfigWidget.h"
#include "ui_RenderConfigWidget.h"

#include <reflectionzeug/PropertyGroup.h>
#include <propertyguizeug/PropertyBrowser.h>

#include <core/data_objects/DataObject.h>
#include <core/AbstractVisualizedData.h>
#include <gui/propertyguizeug_extension/PropertyEditorFactoryEx.h>
#include <gui/propertyguizeug_extension/PropertyPainterEx.h>
#include <gui/data_view/RenderView.h>


using namespace reflectionzeug;
using namespace propertyguizeug;


RenderConfigWidget::RenderConfigWidget(QWidget * parent)
    : QDockWidget(parent)
    , m_ui(new Ui_RenderConfigWidget())
    , m_propertyBrowser(new PropertyBrowser(new PropertyEditorFactoryEx(), new PropertyPainterEx(), this))
    , m_propertyRoot(nullptr)
    , m_renderView(nullptr)
    , m_content(nullptr)
{
    m_ui->setupUi(this);
    m_ui->content->layout()->addWidget(m_propertyBrowser);

    m_propertyBrowser->setAlwaysExpandGroups(true);

    updateTitle();
}

RenderConfigWidget::~RenderConfigWidget()
{
    clear();
    delete m_ui;
}

void RenderConfigWidget::clear()
{
    m_content = nullptr;

    updateTitle();

    m_propertyBrowser->setRoot(nullptr);
    delete m_propertyRoot;
    m_propertyRoot = nullptr;

    emit repaint();
}

void RenderConfigWidget::checkDeletedContent(AbstractVisualizedData * content)
{
    if (m_content == content)
        clear();
}

void RenderConfigWidget::setCurrentRenderView(RenderView * renderView)
{
    clear();

    if (m_renderView)
        disconnect(m_renderView, &RenderView::beforeDeleteContent, this, &RenderConfigWidget::checkDeletedContent);

    m_renderView = renderView;
    m_content = renderView ? renderView->highlightedContent() : nullptr;
    if (renderView)
        connect(renderView, &RenderView::beforeDeleteContent, this, &RenderConfigWidget::checkDeletedContent);
    updateTitle();

    if (!m_content)
        return;

    m_propertyRoot = m_content->createConfigGroup();

    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);

    emit repaint();
}

void RenderConfigWidget::setSelectedData(DataObject * dataObject)
{
    if (!m_renderView)
        return;

    if (!m_renderView->dataObjects().contains(dataObject))
        return;

    AbstractVisualizedData * content = nullptr;
    for (AbstractVisualizedData * it : m_renderView->contents())
    {
        if (it->dataObject() == dataObject)
        {
            content = it;
            break;
        }
    }

    assert(content);

    if (content == m_content)
        return;

    clear();

    m_content = content;
    updateTitle();
    m_propertyRoot = m_content->createConfigGroup();
    m_propertyBrowser->setRoot(m_propertyRoot);
    m_propertyBrowser->setColumnWidth(0, 135);
    emit repaint();
}

void RenderConfigWidget::updateTitle()
{
    QString title;
    if (!m_content)
        title = "(no object selected)";
    else
        title = QString::number(m_renderView->index()) + ": " + m_content->dataObject()->name();

    m_ui->relatedDataObject->setText(title);
}
