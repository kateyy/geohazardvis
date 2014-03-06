#include "application.h"

int main(int argc, char** argv)
{
    Application app(argc, argv);

    app.startup();

    return app.exec();
}