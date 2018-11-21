#ifndef SCREENSHOTRENDERER_H
#define SCREENSHOTRENDERER_H

#include "graphrenderer.h"
#include "graph/graphmodel.h"
#include "graphrenderercore.h"
#include "shared/graph/grapharray.h"

#include <QObject>

class ScreenshotRenderer :
        public QObject,
        public GraphRendererCore
{
    Q_OBJECT

public:
    ScreenshotRenderer(GraphRenderer& renderer);

    void copyTextureObject();
    void updateGPU();
public slots:
    void onPreviewRequested(int width, int height, bool fillSize);
    void onScreenshotRequested(int width, int height, const QString& path, int dpi, bool fillSize);

private:
    GraphModel* _graphModel = nullptr;

    const int TILE_SIZE = 1024;

    GLuint _screenshotFBO = 0;
    GLuint _screenshotTex = 0;

    bool _isScreenshot = false;
    bool _isPreview = false;
    int _viewportWidth = 0;
    int _viewportHeight = 0;
    int _screenshotHeight = 0;
    int _screenshotWidth = 0;
    int _currentTileX = 0;
    int _currentTileY = 0;
    int _tileXCount = 0;
    int _tileYCount = 0;
    QPixmap _fullScreenshot;

    // It's important that these are pointers and not values, because the array will
    // be resized during ComponentManager::update, and we still want to be
    // able to use the existing renderers while this occurs. If the array stored
    // values, then the storage for the renderers themselves would potentially be
    // moved around, as opposed to just the storage for the pointers.
    ComponentArray<MovablePointer<Camera>, LockingGraphArray> _componentCameras;

    enum class Mode
    {
        Overview,
        Component
    };

    SelectionManager* _selectionManager;
    OpenGLDebugLogger _openGLDebugLogger;
    GLuint _sdfTexture;
    bool _FBOcomplete = false;

    GLuint sdfTexture() const override;

    void render();
    void updateComponentGPUData();
signals:
    // Base64 encoded png image for QML...
    void previewComplete(QString previewBase64) const;
    // Screenshot doesn't go to QML so we can use QImage
    void screenshotComplete(const QImage& screenshot, const QString& path) const;
};

#endif // SCREENSHOTRENDERER_H