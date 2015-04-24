#include <core/TextureManager.h>

#include <cassert>

#include <QDebug>
#include <QFileInfo>
#include <QMap>
#include <QString>

#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkImageReader2Factory.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>

#include <core/utility/vtkhelper.h>


namespace
{
struct TextureManager_private
{
    QMap<QString, vtkSmartPointer<vtkImageData>> fileToImage;
    QMap<QString, vtkSmartPointer<vtkTexture>> fileToTexture;
};

TextureManager_private * s_instance = nullptr;
}


void TextureManager::initialize()
{
    assert(!s_instance);
    s_instance = new TextureManager_private;
}

void TextureManager::release()
{
    assert(s_instance);
    delete s_instance;
    s_instance = nullptr;
}

vtkImageData * TextureManager::imageFromFile(const QString & fileName)
{
    assert(s_instance);

    vtkImageData * tex = s_instance->fileToImage.value(fileName);
    if (tex)
        return tex;

    QFileInfo info(fileName);
    if (!info.exists())
    {
        qDebug() << "Texure file not found: " << fileName;
        return nullptr;
    }
    
    QByteArray utf8FN = fileName.toUtf8();

    vtkImageReader2 * reader = vtkImageReader2Factory::CreateImageReader2(utf8FN.data());
    if (!reader)
    {
        qDebug() << "Unsupported image file: " << fileName;
        return nullptr;
    }

    reader->SetFileName(utf8FN.data());
    reader->Update();
    vtkSmartPointer<vtkImageData> image = reader->GetOutput();
    reader->Delete();

    if (!image)
    {
        qDebug() << "Error reading image file: " << fileName;
        return nullptr;
    }

    s_instance->fileToImage.insert(fileName, image);

    return image;
}

vtkTexture * TextureManager::fromFile(const QString & fileName)
{
    assert(s_instance);
    vtkSmartPointer<vtkTexture> tex = s_instance->fileToTexture.value(fileName);
    if (tex)
        return tex;

    vtkImageData * image = imageFromFile(fileName);
    if (!image)
        return nullptr;

    tex = vtkSmartPointer<vtkTexture>::New();
    tex->SetInputData(image);

    s_instance->fileToTexture.insert(fileName, tex);

    return tex;
}
