#include "graphrenderercore.h"

#include "shared/utils/preferences.h"
#include "shadertools.h"

#include <QColor>

template<typename T>
void setupTexture(T t, GLuint& texture, int width, int height, GLint format)
{
    if(texture == 0)
        t->glGenTextures(1, &texture);
    t->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
    t->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
        GraphRendererCore::NUM_MULTISAMPLES, format, width, height, GL_FALSE);
    t->glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, 0);
    t->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

GPUGraphData::GPUGraphData()
{
    resolveOpenGLFunctions();
}

void GPUGraphData::initialise(QOpenGLShaderProgram& nodesShader,
                              QOpenGLShaderProgram& edgesShader,
                              QOpenGLShaderProgram& textShader)
{
    _sphere.setRadius(1.0f);
    _sphere.setRings(16);
    _sphere.setSlices(16);
    _sphere.create(nodesShader);

    _arrow.setRadius(1.0f);
    _arrow.setLength(1.0f);
    _arrow.setSlices(8);
    _arrow.create(edgesShader);

    _rectangle.create(textShader);

    prepareVertexBuffers();
    prepareNodeVAO(nodesShader);
    prepareEdgeVAO(edgesShader);
    prepareTextVAO(textShader);
}

GPUGraphData::~GPUGraphData()
{
    if(_fbo != 0)
    {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    if(_colorTexture != 0)
    {
        glDeleteTextures(1, &_colorTexture);
        _colorTexture = 0;
    }

    if(_selectionTexture != 0)
    {
        glDeleteTextures(1, &_selectionTexture);
        _selectionTexture = 0;
    }
}

void GPUGraphData::prepareVertexBuffers()
{
    if(!_nodeVBO.isCreated())
    {
        _nodeVBO.create();
        _nodeVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }

    if(!_textVBO.isCreated())
    {
        _textVBO.create();
        _textVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }

    if(!_edgeVBO.isCreated())
    {
        _edgeVBO.create();
        _edgeVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }
}

void GPUGraphData::prepareTextVAO(QOpenGLShaderProgram& shader)
{
    _rectangle.vertexArrayObject()->bind();
    shader.bind();
    _textVBO.bind();

    shader.enableAttributeArray("component");
    shader.enableAttributeArray("textureCoord");
    shader.enableAttributeArray("textureLayer");
    shader.enableAttributeArray("basePosition");
    shader.enableAttributeArray("glyphOffset");
    shader.enableAttributeArray("glyphSize");
    shader.enableAttributeArray("color");

    glVertexAttribIPointer(shader.attributeLocation("component"),                                 1, GL_INT, sizeof(GlyphData),
                             reinterpret_cast<const void*>(offsetof(GlyphData, _component))); // NOLINT
    shader.setAttributeBuffer("textureCoord",    GL_FLOAT, offsetof(GlyphData, _textureCoord),    2, sizeof(GlyphData));
    glVertexAttribIPointer(shader.attributeLocation("textureLayer"),                              1, GL_INT, sizeof(GlyphData),
                             reinterpret_cast<const void*>(offsetof(GlyphData, _textureLayer))); // NOLINT
    shader.setAttributeBuffer("basePosition",    GL_FLOAT, offsetof(GlyphData, _basePosition),    3, sizeof(GlyphData));
    shader.setAttributeBuffer("glyphOffset",     GL_FLOAT, offsetof(GlyphData, _glyphOffset),     2, sizeof(GlyphData));
    shader.setAttributeBuffer("glyphSize",       GL_FLOAT, offsetof(GlyphData, _glyphSize),       2, sizeof(GlyphData));
    shader.setAttributeBuffer("color",           GL_FLOAT, offsetof(GlyphData, _color),           3, sizeof(GlyphData));

    glVertexAttribDivisor(shader.attributeLocation("component"), 1);
    glVertexAttribDivisor(shader.attributeLocation("textureCoord"), 1);
    glVertexAttribDivisor(shader.attributeLocation("textureLayer"), 1);
    glVertexAttribDivisor(shader.attributeLocation("basePosition"), 1);
    glVertexAttribDivisor(shader.attributeLocation("glyphOffset"), 1);
    glVertexAttribDivisor(shader.attributeLocation("glyphSize"), 1);
    glVertexAttribDivisor(shader.attributeLocation("color"), 1);

    _textVBO.release();
    shader.release();
    _rectangle.vertexArrayObject()->release();
}

void GPUGraphData::prepareNodeVAO(QOpenGLShaderProgram& shader)
{
    _sphere.vertexArrayObject()->bind();
    shader.bind();

    _nodeVBO.bind();
    shader.enableAttributeArray("nodePosition");
    shader.enableAttributeArray("component");
    shader.enableAttributeArray("size");
    shader.enableAttributeArray("outerColor");
    shader.enableAttributeArray("innerColor");
    shader.enableAttributeArray("outlineColor");
    shader.setAttributeBuffer("nodePosition", GL_FLOAT, offsetof(NodeData, _position),     3,         sizeof(NodeData));
    glVertexAttribIPointer(shader.attributeLocation("component"),                          1, GL_INT, sizeof(NodeData),
                          reinterpret_cast<const void*>(offsetof(NodeData, _component))); // NOLINT
    shader.setAttributeBuffer("size",         GL_FLOAT, offsetof(NodeData, _size),         1,         sizeof(NodeData));
    shader.setAttributeBuffer("outerColor",   GL_FLOAT, offsetof(NodeData, _outerColor),   3,         sizeof(NodeData));
    shader.setAttributeBuffer("innerColor",   GL_FLOAT, offsetof(NodeData, _innerColor),   3,         sizeof(NodeData));
    shader.setAttributeBuffer("outlineColor", GL_FLOAT, offsetof(NodeData, _outlineColor), 3,         sizeof(NodeData));
    glVertexAttribDivisor(shader.attributeLocation("nodePosition"), 1);
    glVertexAttribDivisor(shader.attributeLocation("component"), 1);
    glVertexAttribDivisor(shader.attributeLocation("size"), 1);
    glVertexAttribDivisor(shader.attributeLocation("innerColor"), 1);
    glVertexAttribDivisor(shader.attributeLocation("outerColor"), 1);
    glVertexAttribDivisor(shader.attributeLocation("outlineColor"), 1);
    _nodeVBO.release();

    shader.release();
    _sphere.vertexArrayObject()->release();
}

void GPUGraphData::prepareEdgeVAO(QOpenGLShaderProgram& shader)
{
    _arrow.vertexArrayObject()->bind();
    shader.bind();

    _edgeVBO.bind();
    shader.enableAttributeArray("sourcePosition");
    shader.enableAttributeArray("targetPosition");
    shader.enableAttributeArray("sourceSize");
    shader.enableAttributeArray("targetSize");
    shader.enableAttributeArray("edgeType");
    shader.enableAttributeArray("component");
    shader.enableAttributeArray("size");
    shader.enableAttributeArray("outerColor");
    shader.enableAttributeArray("innerColor");
    shader.enableAttributeArray("outlineColor");
    shader.setAttributeBuffer("sourcePosition", GL_FLOAT, offsetof(EdgeData, _sourcePosition),  3,         sizeof(EdgeData));
    shader.setAttributeBuffer("targetPosition", GL_FLOAT, offsetof(EdgeData, _targetPosition),  3,         sizeof(EdgeData));
    shader.setAttributeBuffer("sourceSize",     GL_FLOAT, offsetof(EdgeData, _sourceSize),      1,         sizeof(EdgeData));
    shader.setAttributeBuffer("targetSize",     GL_FLOAT, offsetof(EdgeData, _targetSize),      1,         sizeof(EdgeData));
    glVertexAttribIPointer(shader.attributeLocation("edgeType"),                                1, GL_INT, sizeof(EdgeData),
                            reinterpret_cast<const void*>(offsetof(EdgeData, _edgeType))); // NOLINT
    glVertexAttribIPointer(shader.attributeLocation("component"),                               1, GL_INT, sizeof(EdgeData),
                           reinterpret_cast<const void*>(offsetof(EdgeData, _component))); // NOLINT
    shader.setAttributeBuffer("size",           GL_FLOAT, offsetof(EdgeData, _size),            1,         sizeof(EdgeData));
    shader.setAttributeBuffer("outerColor",     GL_FLOAT, offsetof(EdgeData, _outerColor),      3,         sizeof(EdgeData));
    shader.setAttributeBuffer("innerColor",     GL_FLOAT, offsetof(EdgeData, _innerColor),      3,         sizeof(EdgeData));
    shader.setAttributeBuffer("outlineColor",   GL_FLOAT, offsetof(EdgeData, _outlineColor),    3,         sizeof(EdgeData));
    glVertexAttribDivisor(shader.attributeLocation("sourcePosition"),   1);
    glVertexAttribDivisor(shader.attributeLocation("targetPosition"),   1);
    glVertexAttribDivisor(shader.attributeLocation("sourceSize"),       1);
    glVertexAttribDivisor(shader.attributeLocation("targetSize"),       1);
    glVertexAttribDivisor(shader.attributeLocation("edgeType"),         1);
    glVertexAttribDivisor(shader.attributeLocation("component"),        1);
    glVertexAttribDivisor(shader.attributeLocation("size"),             1);
    glVertexAttribDivisor(shader.attributeLocation("outerColor"),       1);
    glVertexAttribDivisor(shader.attributeLocation("innerColor"),       1);
    glVertexAttribDivisor(shader.attributeLocation("outlineColor"),     1);
    _edgeVBO.release();

    shader.release();
    _arrow.vertexArrayObject()->release();
}

bool GPUGraphData::prepareRenderBuffers(int width, int height, GLuint depthTexture)
{
    setupTexture(this, _colorTexture,     width, height, GL_RGBA);
    setupTexture(this, _selectionTexture, width, height, GL_RGBA);

    if(_fbo == 0)
        glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, _colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, _selectionTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D_MULTISAMPLE, depthTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool fboValid = (status == GL_FRAMEBUFFER_COMPLETE);
    Q_ASSERT(fboValid);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return fboValid;
}

void GPUGraphData::reset()
{
    _alpha1 = 0.0f;
    _alpha2 = 0.0f;
    _nodeData.clear();
    _edgeData.clear();
    _glyphData.clear();
}

void GPUGraphData::clearFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, static_cast<GLenum*>(drawBuffers));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GPUGraphData::clearDepthbuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GPUGraphData::upload()
{
    _nodeVBO.bind();
    _nodeVBO.allocate(_nodeData.data(), static_cast<int>(_nodeData.size()) * sizeof(NodeData));
    _nodeVBO.release();

    _edgeVBO.bind();
    _edgeVBO.allocate(_edgeData.data(), static_cast<int>(_edgeData.size()) * sizeof(EdgeData));
    _edgeVBO.release();

    _textVBO.bind();
    _textVBO.allocate(_glyphData.data(), static_cast<int>(_glyphData.size()) * sizeof(GlyphData));
    _textVBO.release();
}

int GPUGraphData::numNodes() const
{
    return static_cast<int>(_nodeData.size());
}

int GPUGraphData::numEdges() const
{
    return static_cast<int>(_edgeData.size());
}

float GPUGraphData::alpha() const
{
    return _alpha1 * _alpha2;
}

bool GPUGraphData::unused() const
{
    return _alpha1 == 0.0f && _alpha2 == 0.0f;
}

GraphRendererCore::GraphRendererCore() :
    OpenGLFunctions()
{
    resolveOpenGLFunctions();

    ShaderTools::loadShaderProgram(_screenShader, QStringLiteral(":/shaders/screen.vert"), QStringLiteral(":/shaders/screen.frag"));
    ShaderTools::loadShaderProgram(_selectionShader, QStringLiteral(":/shaders/screen.vert"), QStringLiteral(":/shaders/selection.frag"));

    ShaderTools::loadShaderProgram(_nodesShader, QStringLiteral(":/shaders/instancednodes.vert"), QStringLiteral(":/shaders/nodecolorads.frag"));
    ShaderTools::loadShaderProgram(_edgesShader, QStringLiteral(":/shaders/instancededges.vert"), QStringLiteral(":/shaders/edgecolorads.frag"));

    ShaderTools::loadShaderProgram(_selectionMarkerShader, QStringLiteral(":/shaders/2d.vert"), QStringLiteral(":/shaders/selectionMarker.frag"));

    ShaderTools::loadShaderProgram(_textShader, QStringLiteral(":/shaders/textrender.vert"), QStringLiteral(":/shaders/textrender.frag"));

    for(auto& gpuGraphData : _gpuGraphData)
        gpuGraphData.initialise(_nodesShader, _edgesShader, _textShader);

    prepareComponentDataTexture();
    prepareSelectionMarkerVAO();
    prepareQuad();
}

GraphRendererCore::~GraphRendererCore()
{
    if(_componentDataTBO != 0)
    {
        glDeleteBuffers(1, &_componentDataTBO);
        _componentDataTBO = 0;
    }

    if(_componentDataTexture != 0)
    {
        glDeleteTextures(1, &_componentDataTexture);
        _componentDataTexture = 0;
    }

    if(_depthTexture != 0)
    {
        glDeleteTextures(1, &_depthTexture);
        _depthTexture = 0;
    }
}

static void setShaderADSParameters(QOpenGLShaderProgram& program)
{
    struct Light
    {
        Light() = default;
        Light(const QVector4D& _position, const QVector3D& _intensity) :
            position(_position), intensity(_intensity)
        {}

        QVector4D position;
        QVector3D intensity;
    };

    std::vector<Light> lights;
    lights.emplace_back(QVector4D(-20.0f, 0.0f, 3.0f, 1.0f), QVector3D(0.6f, 0.6f, 0.6f));
    lights.emplace_back(QVector4D(0.0f, 0.0f, 0.0f, 1.0f), QVector3D(0.2f, 0.2f, 0.2f));
    lights.emplace_back(QVector4D(10.0f, -10.0f, -10.0f, 1.0f), QVector3D(0.4f, 0.4f, 0.4f));

    auto numberOfLights = static_cast<int>(lights.size());

    program.setUniformValue("numberOfLights", numberOfLights);

    for(int i = 0; i < numberOfLights; i++)
    {
        QByteArray positionId = QStringLiteral("lights[%1].position").arg(i).toLatin1();
        program.setUniformValue(positionId.data(), lights[i].position);

        QByteArray intensityId = QStringLiteral("lights[%1].intensity").arg(i).toLatin1();
        program.setUniformValue(intensityId.data(), lights[i].intensity);
    }

    program.setUniformValue("material.ks", QVector3D(1.0f, 1.0f, 1.0f));
    program.setUniformValue("material.ka", QVector3D(0.02f, 0.02f, 0.02f));
    program.setUniformValue("material.shininess", 50.0f);
}

void GraphRendererCore::renderNodes(GPUGraphData& gpuGraphData)
{
    _nodesShader.bind();
    setShaderADSParameters(_nodesShader);

    gpuGraphData._nodeVBO.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, _componentDataTexture);
    _nodesShader.setUniformValue("componentData", 0);

    gpuGraphData._sphere.vertexArrayObject()->bind();
    glDrawElementsInstanced(GL_TRIANGLES, gpuGraphData._sphere.glIndexCount(),
                            GL_UNSIGNED_INT, nullptr, gpuGraphData.numNodes());
    gpuGraphData._sphere.vertexArrayObject()->release();

    glBindTexture(GL_TEXTURE_BUFFER, 0);
    gpuGraphData._nodeVBO.release();
    _nodesShader.release();
}

void GraphRendererCore::renderEdges(GPUGraphData& gpuGraphData)
{
    _edgesShader.bind();
    setShaderADSParameters(_edgesShader);

    gpuGraphData._edgeVBO.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, _componentDataTexture);
    _edgesShader.setUniformValue("componentData", 0);

    gpuGraphData._arrow.vertexArrayObject()->bind();
    glDrawElementsInstanced(GL_TRIANGLES, gpuGraphData._arrow.glIndexCount(),
                            GL_UNSIGNED_INT, nullptr, gpuGraphData.numEdges());
    gpuGraphData._arrow.vertexArrayObject()->release();

    glBindTexture(GL_TEXTURE_BUFFER, 0);
    gpuGraphData._edgeVBO.release();
    _edgesShader.release();
}

void GraphRendererCore::renderText(GPUGraphData& gpuGraphData)
{
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    _textShader.bind();
    gpuGraphData._textVBO.bind();

     // Bind SDF textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, sdfTexture());

    // Set to linear filtering for SDF text
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    _textShader.setUniformValue("tex", 0);
    _textShader.setUniformValue("textScale", u::pref("visuals/textSize").toFloat());

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_BUFFER, _componentDataTexture);
    _textShader.setUniformValue("componentData", 1);

    gpuGraphData._rectangle.vertexArrayObject()->bind();
    glDrawElementsInstanced(GL_TRIANGLES, gpuGraphData._rectangle.glIndexCount(),
                            GL_UNSIGNED_INT, nullptr, static_cast<int>(gpuGraphData._glyphData.size())) ;
    gpuGraphData._rectangle.vertexArrayObject()->release();

    glBindTexture(GL_TEXTURE_BUFFER, 0);

    gpuGraphData._textVBO.release();
    _textShader.release();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

GPUGraphData* GraphRendererCore::gpuGraphDataForAlpha(float alpha1, float alpha2)
{
    for(auto& gpuGraphData : _gpuGraphData)
    {
        if(gpuGraphData.unused())
        {
            gpuGraphData._alpha1 = alpha1;
            gpuGraphData._alpha2 = alpha2;
            return &gpuGraphData;
        }

        if(gpuGraphData._alpha1 == alpha1 && gpuGraphData._alpha2 == alpha2)
            return &gpuGraphData;
    }

    qWarning() << "Not enough gpuGraphData instances for" << alpha1 << alpha2;
    for(auto& gpuGraphData : _gpuGraphData)
        qWarning() << "  " << gpuGraphData._alpha1 << gpuGraphData._alpha2;

    return nullptr;
}

void GraphRendererCore::resetGPUGraphData()
{
    for(auto& gpuGraphData : _gpuGraphData)
        gpuGraphData.reset();
}

void GraphRendererCore::uploadGPUGraphData()
{
    for(auto& gpuGraphData : _gpuGraphData)
    {
        if(gpuGraphData.alpha() > 0.0f)
            gpuGraphData.upload();
    }
}

bool GraphRendererCore::resize(int width, int height)
{
    _width = width;
    _height = height;

    bool FBOcomplete = false;

    if(width > 0 && height > 0)
    {
        setupTexture(this, _depthTexture, width, height, GL_DEPTH_COMPONENT);

        if(!_gpuGraphData.empty())
        {
            FBOcomplete = true;
            for(auto& gpuGraphData : _gpuGraphData)
            {
                FBOcomplete = FBOcomplete &&
                    gpuGraphData.prepareRenderBuffers(width, height, _depthTexture);
            }
        }
        else
            FBOcomplete = false;
    }

    auto w = static_cast<GLfloat>(_width);
    auto h = static_cast<GLfloat>(_height);
    GLfloat quadData[] =
    {
        0, 0,
        w, 0,
        w, h,

        w, h,
        0, h,
        0, 0,
    };

    _screenQuadDataBuffer.bind();
    _screenQuadDataBuffer.allocate(static_cast<void*>(quadData), static_cast<int>(sizeof(quadData)));
    _screenQuadDataBuffer.release();

    return FBOcomplete;
}

std::vector<int> GraphRendererCore::gpuGraphDataRenderOrder() const
{
    std::vector<int> renderOrder;
    renderOrder.reserve(_gpuGraphData.size());

    for(int i = 0; i < static_cast<int>(_gpuGraphData.size()); i++)
        renderOrder.push_back(i);

    std::sort(renderOrder.begin(), renderOrder.end(), [this](auto a, auto b)
    {
        if(_gpuGraphData.at(a)._alpha1 == _gpuGraphData.at(b)._alpha1)
            return _gpuGraphData.at(a)._alpha2 > _gpuGraphData.at(b)._alpha2;

        return _gpuGraphData.at(a)._alpha1 > _gpuGraphData.at(b)._alpha1;
    });

    while(!renderOrder.empty() && _gpuGraphData.at(renderOrder.back()).alpha() <= 0.0f)
        renderOrder.pop_back();

    return renderOrder;
}

void GraphRendererCore::renderGraph()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_BLEND);

    if(hasSampleShading())
    {
        // Enable per-sample shading, this makes small text look nice
        glEnable(GL_SAMPLE_SHADING_ARB);
    }

    for(auto& gpuGraphData : _gpuGraphData)
        gpuGraphData.clearFramebuffer();

    for(auto i : gpuGraphDataRenderOrder())
    {
        auto& gpuGraphData = _gpuGraphData.at(i);

        // Clear the depth buffer, but only when we're about to render graph elements
        // that are found, so that subsequent render passes of not found elements
        // use the existing depth information
        if(gpuGraphData._alpha2 >= 1.0f)
            gpuGraphData.clearDepthbuffer();

        if(hasSampleShading())
        {
            // Shade all samples in multi-sampling
            glMinSampleShading(1.0f);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, gpuGraphData._fbo);

        GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, static_cast<GLenum*>(drawBuffers));

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderNodes(gpuGraphData);
        renderEdges(gpuGraphData);
        renderText(gpuGraphData);
    }

    glDisable(GL_SAMPLE_SHADING);
    glDisable(GL_MULTISAMPLE);
}

void GraphRendererCore::render2D(QRect selectionRect)
{
    const auto& renderOrder = gpuGraphDataRenderOrder();
    auto index = !renderOrder.empty() ? renderOrder.front() : 0;
    auto& gpuGraphData = _gpuGraphData.at(index);

    if(gpuGraphData.unused())
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, gpuGraphData._fbo);

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, _width, _height);

    QMatrix4x4 m;
    m.ortho(0.0f, _width, 0.0f, _height, -1.0f, 1.0f);

    if(!selectionRect.isNull())
    {
        const QColor color(Qt::white);

        QRect r;
        r.setLeft(selectionRect.left());
        r.setRight(selectionRect.right());
        r.setTop(_height - selectionRect.top());
        r.setBottom(_height - selectionRect.bottom());

        std::vector<GLfloat> quadData;

        quadData.push_back(r.left()); quadData.push_back(r.bottom());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());
        quadData.push_back(r.right()); quadData.push_back(r.bottom());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());
        quadData.push_back(r.right()); quadData.push_back(r.top());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());

        quadData.push_back(r.right()); quadData.push_back(r.top());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());
        quadData.push_back(r.left());  quadData.push_back(r.top());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());
        quadData.push_back(r.left());  quadData.push_back(r.bottom());
        quadData.push_back(color.redF()); quadData.push_back(color.blueF()); quadData.push_back(color.greenF());

        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        _selectionMarkerDataBuffer.bind();
        _selectionMarkerDataBuffer.allocate(quadData.data(), static_cast<int>(quadData.size()) * sizeof(GLfloat));

        _selectionMarkerShader.bind();
        _selectionMarkerShader.setUniformValue("projectionMatrix", m);

        _selectionMarkerDataVAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        _selectionMarkerDataVAO.release();

        _selectionMarkerShader.release();
        _selectionMarkerDataBuffer.release();
    }

    glEnable(GL_DEPTH_TEST);
}

static void render2DComposite(OpenGLFunctions& f, QOpenGLShaderProgram& shader, GLuint texture, float alpha)
{
    shader.bind();
    shader.setUniformValue("alpha", alpha);
    f.glActiveTexture(GL_TEXTURE0);
    f.glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
    f.glDrawArrays(GL_TRIANGLES, 0, 6);
    shader.release();
}

void GraphRendererCore::renderToFramebuffer()
{
    glViewport(0, 0, _width, _height);


    auto backgroundColor = u::pref("visuals/backgroundColor").value<QColor>();

    glClearColor(backgroundColor.redF(),
                 backgroundColor.greenF(),
                 backgroundColor.blueF(), 1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    QMatrix4x4 m;
    m.ortho(0, _width, 0, _height, -1.0f, 1.0f);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glEnable(GL_BLEND);

    _screenShader.bind();
    _screenShader.setUniformValue("projectionMatrix", m);
    _screenShader.release();

    _selectionShader.bind();
    _selectionShader.setUniformValue("projectionMatrix", m);
    _selectionShader.setUniformValue("highlightColor",
                                     u::pref("visuals/highlightColor").value<QColor>());
    _selectionShader.release();

    _screenQuadDataBuffer.bind();
    _screenQuadVAO.bind();

    for(auto i : gpuGraphDataRenderOrder())
    {
        render2DComposite(*this, _screenShader,    _gpuGraphData.at(i)._colorTexture,     _gpuGraphData.at(i).alpha());
        render2DComposite(*this, _selectionShader, _gpuGraphData.at(i)._selectionTexture, _gpuGraphData.at(i).alpha());
    }

    _screenQuadDataBuffer.release();
    _screenQuadVAO.release();
}

void GraphRendererCore::prepareSelectionMarkerVAO()
{
    _selectionMarkerDataVAO.create();

    _selectionMarkerDataVAO.bind();
    _selectionMarkerShader.bind();

    _selectionMarkerDataBuffer.create();
    _selectionMarkerDataBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    _selectionMarkerDataBuffer.bind();

    _selectionMarkerShader.enableAttributeArray("position");
    _selectionMarkerShader.enableAttributeArray("color");
    _selectionMarkerShader.disableAttributeArray("texCoord");
    _selectionMarkerShader.setAttributeBuffer("position", GL_FLOAT, 0, 2, 5 * sizeof(GLfloat));
    _selectionMarkerShader.setAttributeBuffer("color", GL_FLOAT, 2 * sizeof(GLfloat), 3, 5 * sizeof(GLfloat));

    _selectionMarkerDataBuffer.release();
    _selectionMarkerDataVAO.release();
    _selectionMarkerShader.release();
}

void GraphRendererCore::prepareQuad()
{
    if(!_screenQuadVAO.isCreated())
        _screenQuadVAO.create();

    _screenQuadVAO.bind();

    _screenQuadDataBuffer.create();
    _screenQuadDataBuffer.bind();
    _screenQuadDataBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    _screenShader.bind();
    _screenShader.enableAttributeArray("position");
    _screenShader.setAttributeBuffer("position", GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    _screenShader.setUniformValue("frameBufferTexture", 0);
    _screenShader.setUniformValue("multisamples", NUM_MULTISAMPLES);
    _screenShader.release();

    _selectionShader.bind();
    _selectionShader.enableAttributeArray("position");
    _selectionShader.setAttributeBuffer("position", GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    _selectionShader.setUniformValue("frameBufferTexture", 0);
    _selectionShader.setUniformValue("multisamples", NUM_MULTISAMPLES);
    _selectionShader.release();

    _screenQuadDataBuffer.release();
    _screenQuadVAO.release();
}

void GraphRendererCore::prepareComponentDataTexture()
{
    if(_componentDataTexture == 0)
        glGenTextures(1, &_componentDataTexture);

    if(_componentDataTBO == 0)
        glGenBuffers(1, &_componentDataTBO);

    glBindTexture(GL_TEXTURE_BUFFER, _componentDataTexture);
    glBindBuffer(GL_TEXTURE_BUFFER, _componentDataTBO);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _componentDataTBO);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}