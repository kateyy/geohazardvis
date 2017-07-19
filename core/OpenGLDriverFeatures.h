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

#include <core/config.h>
#include <core/core_api.h>


class QString;
class vtkGenericOpenGLRenderWindow;


/**
 * Retrieve OpenGL renderer and context information, and work around known driver bugs.
 */
class CORE_API OpenGLDriverFeatures
{
public:
    /**
     * Fetch renderer/driver/context features from the current OpenGL context.
     * Make sure that there is a current OpenGL context when calling this function!
     * This has only an effect when called for the first time.
     */
    static void initializeInCurrentContext();

    static void printDebugInfo();
    static const QString & vendor();
    static int openGLVersionMajor();
    static int openGLVersionMinor();

    /**
     * @return Maximum supported line width that can safely be renderer with VTK on the current
     * renderer. This is used to prevent VTK from using geometry shaders on Intel HD 3000 etc.
     * If there is no limit on the current platform, 0.f is returned.
     */
    static float maxSupportedLineWidth();
    /** Convenience method returning a desired maximum width reduced to the supported maximum. */
    static float clampToMaxSupportedLineWidth(float desiredMaxWidth);
    static unsigned int clampToMaxSupportedLineWidth(unsigned int desiredMaxWidth);

    /** This must be called after paintGL in Qt OpenGL widgets. */
    static void setFeaturesAfterPaintGL(vtkGenericOpenGLRenderWindow * renderWindow);

private:
    OpenGLDriverFeatures() = delete;
    ~OpenGLDriverFeatures() = delete;
};
