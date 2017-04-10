#define NOMINMAX

#include "OpenGLDriverFeatures.h"

#include <algorithm>

#include <QDebug>
#include <QString>

#include <vtk_glew.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <core/config.h>


namespace
{

struct Features_p
{
    bool isInitialized = false;
    GLint major = -1;
    GLint minor = -1;
    QString vendor;
    QString renderer;
    QString driverVersion;

    float maxSupportedLineWidth = 0.f;

    bool applyIntelHD2k_3kHack = false;
};

Features_p & pFeatures()
{
    static Features_p pf;
    return pf;
}

}

void OpenGLDriverFeatures::initializeInCurrentContext()
{
    if (pFeatures().isInitialized)
    {
        return;
    }
    pFeatures().isInitialized = true;

#if VTK_RENDERING_BACKEND == 2
    glGetIntegerv(GL_MAJOR_VERSION, &pFeatures().major);
    glGetIntegerv(GL_MINOR_VERSION, &pFeatures().minor);

    pFeatures().vendor = QString::fromLatin1(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
    pFeatures().renderer = QString::fromLatin1(reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    const auto error = glGetError();
    if (error != GL_NO_ERROR)
    {
        qDebug() << "Error occurred while retrieving OpenGL context and driver information";
    }
    printDebugInfo();

#if WIN32
    const auto & renderer = pFeatures().renderer;
    if (pFeatures().vendor == "Intel"
        && (renderer == "Intel(R) HD Graphics"
            || renderer == "Intel(R) HD Graphics 2000"
            || renderer == "Intel(R) HD Graphics 3000"))
    {
        pFeatures().maxSupportedLineWidth = 7.f;
        pFeatures().applyIntelHD2k_3kHack = true;
    }
#endif

#endif
}

void OpenGLDriverFeatures::printDebugInfo()
{
    qDebug().noquote().nospace() << "OpenGL Context Information:";
    qDebug().noquote().nospace() << "Version:  " << pFeatures().major << "." << pFeatures().minor;
    qDebug().noquote().nospace() << "Vendor:   " << pFeatures().vendor;
    qDebug().noquote().nospace() << "Renderer: " << pFeatures().renderer;
}

const QString & OpenGLDriverFeatures::vendor()
{
    return pFeatures().vendor;
}

unsigned int OpenGLDriverFeatures::openGLVersionMajor()
{
    return pFeatures().major;
}

unsigned int OpenGLDriverFeatures::openGLVersionMinor()
{
    return pFeatures().minor;
}

float OpenGLDriverFeatures::maxSupportedLineWidth()
{
    return pFeatures().maxSupportedLineWidth;
}

float OpenGLDriverFeatures::clampToMaxSupportedLineWidth(const float desiredMaxWidth)
{
    return pFeatures().maxSupportedLineWidth != 0.f
        ? std::min(pFeatures().maxSupportedLineWidth, desiredMaxWidth)
        : desiredMaxWidth;
}

unsigned int OpenGLDriverFeatures::clampToMaxSupportedLineWidth(unsigned int desiredMaxWidth)
{
    if (pFeatures().maxSupportedLineWidth == 0.f)
    {
        return desiredMaxWidth;
    }

    return static_cast<unsigned int>(std::floor(
        std::min(pFeatures().maxSupportedLineWidth, static_cast<float>(desiredMaxWidth))));
}

void OpenGLDriverFeatures::setFeaturesAfterPaintGL(vtkGenericOpenGLRenderWindow * renderWindow)
{
    if (!renderWindow)
    {
        return;
    }

#if VTK_RENDERING_BACKEND == 2
    if (pFeatures().applyIntelHD2k_3kHack)
    {
        renderWindow->SetForceMaximumHardwareLineWidth(7.f);
    }
#endif
}
