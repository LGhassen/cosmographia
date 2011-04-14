// This file is part of Cosmographia.
//
// Copyright (C) 2011 Chris Laurel <claurel@gmail.com>
//
// Cosmographia is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Cosmographia is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Cosmographia. If not, see <http://www.gnu.org/licenses/>.

#include "CmodLoader.h"
#include <QDataStream>
#include <QVector>

using namespace vesta;


CmodLoader::CmodLoader(QIODevice *in) :
    m_inputStream(NULL)
{
    m_inputStream = new QDataStream(in);
    m_inputStream->setByteOrder(QDataStream::LittleEndian);
}


CmodLoader::~CmodLoader()
{
    delete m_inputStream;
}


void
CmodLoader::setError(const QString& errorMessage)
{
    m_errorMessage = errorMessage;
    m_hasError = true;
}


vesta::Material*
CmodLoader::loadMaterial()
{
    Material* material = new Material();
    material->setDiffuse(Spectrum::Black());
    material->setSpecular(Spectrum::Black());
    material->setEmission(Spectrum::Black());
    material->setOpacity(1.0f);
    material->setBrdf(Material::Lambert);
    material->setPhongExponent(1.0f);

    bool done = false;
    while (!done && !error())
    {
        CmodToken token = readCmodToken();
        switch (token)
        {
        case CmodDiffuse:
        {
            Spectrum diffuseColor;
            if (!readColorProperty(&diffuseColor))
            {
                setError("Error reading diffuse color in material");
            }
            else
            {
                material->setDiffuse(diffuseColor);
            }
        }
        break;

        case CmodSpecular:
        {
            Spectrum specularColor;
            if (!readColorProperty(&specularColor))
            {
                setError("Error reading specular color in material");
            }
            else
            {
                material->setSpecular(specularColor);
            }
        }
        break;

        case CmodSpecularPower:
        {
            float specularPower = 0.0f;
            if (!readFloat1Property(&specularPower))
            {
                setError("Error reading specular power");
            }
            else
            {
                material->setPhongExponent(specularPower);
            }
        }
        break;

        case CmodEmissive:
        {
            Spectrum emissiveColor;
            if (!readColorProperty(&emissiveColor))
            {
                setError("Error reading emissive color in material");
            }
            else
            {
                material->setEmission(emissiveColor);
            }
        }
        break;

        case CmodOpacity:
        {
            float opacity = 0.0f;
            if (!readFloat1Property(&opacity))
            {
                setError("Error reading material opacity");
            }
            else
            {
                material->setOpacity(opacity);
            }
        }
        break;

        case CmodTexture:
        {
            quint16 textureType = 0;
            QString textureSource;
            *m_inputStream >> textureType;
            if (!readStringProperty(&textureSource))
            {
                setError("Error reading texture source");
            }
        }
        break;

        case CmodBlend:
        {
            quint16 blendMode = 0;
            *m_inputStream >> blendMode;
        }
        break;

        case CmodEndMaterial:
            done = true;
            break;

        default:
            setError("Unrecognized token in material");
            break;
        }
    }

    if (error())
    {
        delete material;
        material = NULL;
    }

    return material;
}


VertexArray*
CmodLoader::loadVertexArray(const VertexSpec& spec)
{
    if (readCmodToken() != CmodVertices)
    {
        setError("Vertex data missing");
        return NULL;
    }

    quint32 vertexCount = 0;
    *m_inputStream >> vertexCount;

    if (vertexCount == 0)
    {
        setError("Vertex data block has zero vertices");
        return NULL;
    }

    unsigned int dataSize = vertexCount * spec.size();
    char* vertexData = new char[dataSize];
    if (!vertexData)
    {
        setError("Error allocating vertex data for mesh (out of memory)");
        return NULL;
    }

    for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {

    }

    return new VertexArray(vertexData, vertexCount, spec);
}


Submesh*
CmodLoader::loadSubmesh()
{
    VertexSpec* vertexSpec = loadVertexSpec();
    if (!vertexSpec)
    {
        return NULL;
    }

    VertexArray* vertexArray = loadVertexArray(*vertexSpec);
    if (!vertexArray)
    {
        delete vertexSpec;
        return NULL;
    }

    Submesh* submesh = new Submesh(vertexArray);

    bool done = false;
    while (!done && !error())
    {
        quint16 token = 0;
        *m_inputStream >> token;

        if (token == CmodEndMesh)
        {
            done = true;
        }
        else
        {

        }
    }

    return submesh;
}


VertexSpec*
CmodLoader::loadVertexSpec()
{
    if (readCmodToken() != CmodVertexDesc)
    {
        setError("Missing vertex description in submesh");
        return NULL;
    }

    QVector<VertexAttribute> attributes;
    bool done = false;

    while (!done && !error())
    {
        quint16 token = 0;
        *m_inputStream >> token;
        if (token == CmodEndVertexDesc)
        {
            done = true;
        }
        else
        {
            VertexAttribute::Semantic semantic = VertexAttribute::InvalidAttributeSemantic;
            switch (token)
            {
            case 0:
                semantic = VertexAttribute::Position;
                break;
            case 1:
                semantic = VertexAttribute::Color;
                break;
            case 2:
                setError("color1 is not a supported vertex attribute");
                break;
            case 3:
                semantic = VertexAttribute::Normal;
                break;
            case 4:
                semantic = VertexAttribute::Tangent;
                break;
            case 5:
                semantic = VertexAttribute::TextureCoord;
                break;
            case 6:
                setError("texture1 is not a supported vertex attribute");
                break;
            case 7:
                setError("texture2 is not a supported vertex attribute");
                break;
            case 8:
                setError("texture3 is not a supported vertex attribute");
                break;
            case 9:
                semantic = VertexAttribute::PointSize;
                break;
            default:
                setError("Unrecognized vertex attribute semantic");
                break;
            }

            VertexAttribute::Format format = VertexAttribute::InvalidAttributeFormat;
            quint16 formatIndex = 0;
            *m_inputStream >> formatIndex;
            switch (formatIndex)
            {
            case 0:
                format = VertexAttribute::Float1;
                break;
            case 1:
                format = VertexAttribute::Float2;
                break;
            case 2:
                format = VertexAttribute::Float3;
                break;
            case 3:
                format = VertexAttribute::Float4;
                break;
            case 4:
                format = VertexAttribute::UByte4;
                break;
            default:
                setError("Unrecognized vertex attribute format");
                break;
            }

            if (format != VertexAttribute::InvalidAttributeFormat &&
                semantic != VertexAttribute::InvalidAttributeSemantic)
            {
                attributes << VertexAttribute(semantic, format);
            }
        }
    }

    if (attributes.isEmpty())
    {
        setError("Submesh has an empty vertex description");
        return NULL;
    }

    return new VertexSpec(attributes.size(), attributes.data());
}


MeshGeometry*
CmodLoader::loadMesh()
{
    char headerData[16];
    if (m_inputStream->readRawData(headerData, sizeof(headerData)) != sizeof(headerData))
    {
        setError("Error reading header");
        return NULL;
    }

    QString header = QString::fromAscii(headerData, sizeof(headerData));
    if (header == "#celmodel__ascii")
    {
        setError("ASCII cmod files are not supported");
        return NULL;
    }
    else if (header != "#celmodel_binary")
    {
        setError("Wrong header; file is not a cmod mesh file");
        return NULL;
    }

    MeshGeometry* mesh = new MeshGeometry();

    bool done = false;
    unsigned int meshCount = 0;

    while (!done && !error())
    {
        CmodToken token = readCmodToken();
        if (m_inputStream->atEnd())
        {
            done = true;
        }

        if (token == CmodMaterial)
        {
            if (meshCount > 0)
            {
                setError("Materials must be appear before any meshes");
            }

            Material* material = loadMaterial();
            if (material)
            {
                mesh->addMaterial(material);
            }
        }
        else if (token == CmodMesh)
        {
            Submesh* submesh = loadSubmesh();
            if (submesh)
            {
                mesh->addSubmesh(submesh);
            }
        }
        else
        {
            setError("Unrecognized block in cmod (not a mesh or material)");
        }
    }

    if (error())
    {
        delete mesh;
        mesh = NULL;
    }

    return mesh;
}


bool
CmodLoader::readFloat1Property(float* value)
{
    CmodDataType type = readCmodDataType();
    if (type != CmodFloat1)
    {
        return false;
    }
    else
    {
        *m_inputStream >> *value;
        return true;
    }
}


bool
CmodLoader::readColorProperty(vesta::Spectrum* value)
{
    CmodDataType type = readCmodDataType();
    if (type != CmodColor)
    {
        return false;
    }
    else
    {
        float red = 0.0f, green = 0.0f, blue = 0.0f;
        *m_inputStream >> red >> green >> blue;
        *value = Spectrum(red, green, blue);
        return true;
    }
}


bool
CmodLoader::readStringProperty(QString* value)
{
    if (readCmodDataType() != CmodString)
    {
        return false;
    }
    else
    {
        quint16 stringLength = 0;
        *m_inputStream >> stringLength;
        char* data = new char[stringLength];
        if (m_inputStream->readRawData(data, stringLength) != stringLength)
        {
            delete data;
            return false;
        }
        else
        {
            *value = QString::fromUtf8(data, stringLength);
            delete data;
            return true;
        }
    }
}


CmodLoader::CmodToken
CmodLoader::readCmodToken()
{
    quint16 tokenValue = 0;
    *m_inputStream >> tokenValue;
    return CmodToken(tokenValue);
}


CmodLoader::CmodDataType
CmodLoader::readCmodDataType()
{
    quint16 typeValue = 0;
    *m_inputStream >> typeValue;
    return CmodDataType(typeValue);
}

