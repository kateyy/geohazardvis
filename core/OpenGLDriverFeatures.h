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
