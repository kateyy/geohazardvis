#include <QApplication>

#include "gui/viewer.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Viewer viewer;
    viewer.show();

    return app.exec();    
}