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

#include "OpenGLDriverFeatures.h"

#include <algorithm>

#include <QDebug>
#include <QString>

#include <core/config.h>

#if VTK_RENDERING_BACKEND == 2
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <vtkGenericOpenGLRenderWindow.h>
#endif

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

#if VTK_RENDERING_BACKEND == 1

    pFeatures().isInitialized = true;

#elif VTK_RENDERING_BACKEND == 2

    const auto context = QOpenGLContext::currentContext();
    const auto functions = context ? context->functions() : static_cast<QOpenGLFunctions *>(nullptr);
    assert(functions);
    if (!functions)
    {
        qDebug() << "[OpenGLDriverFeatures] OpenGL context or Qt OpenGL functions not initialized.";
        return;
    }

    pFeatures().isInitialized = true;
    
    auto & f = *functions;
    f.glGetIntegerv(GL_MAJOR_VERSION, &pFeatures().major);
    f.glGetIntegerv(GL_MINOR_VERSION, &pFeatures().minor);
    auto vendorString = reinterpret_cast<const char *>(f.glGetString(GL_VENDOR));
    pFeatures().vendor = QString::fromLatin1(vendorString ? vendorString : "");
    auto rendererString = reinterpret_cast<const char *>(f.glGetString(GL_RENDERER));
    pFeatures().renderer = QString::fromLatin1(rendererString ? rendererString : "");

    const auto error = f.glGetError();
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

int OpenGLDriverFeatures::openGLVersionMajor()
{
    return pFeatures().major;
}

int OpenGLDriverFeatures::openGLVersionMinor()
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
