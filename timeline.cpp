#include <QtWidgets>

#include "timeline.h"
#include <QDebug>
#include <QPoint>
#include <QRect>



Timeline::Timeline(QWidget *parent) : QWidget(parent)
{
    defined = false;
    setAttribute(Qt::WA_StaticContents);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    backgroundColor = QWidget::palette().color(QWidget::backgroundRole());
    foregroundColor = QWidget::palette().color(QWidget::foregroundRole());



    rubberBandShown = false;
    image = QImage(size(), QImage::Format_ARGB32);
    image.fill(Qt::black);

    zoomFactor = 2.0f;
    scrollPos = 4.75;

    refreshPixmap();
}

/**
 * @brief Timeline::init Initialises the widget with a given number of frames;
 * theses start off as empty so be sure to update their thumbnails.
 * @param frameCount
 * @param frameSize The size in pixels of the thumbnails, must stay the same
 * across every frame.
 */
void Timeline::init(int frameCount, QSize frameSize)
{
    this->frameSize = frameSize;
    this->thumbnails.resize(frameCount);
    defined = true;
}





/**
 * @brief Timeline::checkSize checks whether the supplied image is compatible
 * with this timeline
 * @param image
 * @return bool
 */
bool Timeline::checkSize(const QImage &image) {
    if (!defined)
    {
        return false;
    }

    return image.size() == frameSize;
}



void Timeline::setImage(int index, const QImage &image)
{
    thumbnails.at(index) = image;
    refreshPixmap();
}

void Timeline::insertImage(int index, const QImage &image)
{
    //thumbnails.insert(image, index);
}

void Timeline::updateImage(int index, const QImage &image)
{
    thumbnails.at(index) = image;
    refreshPixmap();
}

void Timeline::removeImage(int index)
{
    thumbnails.at(index) = QImage();
    refreshPixmap();
}


QSize Timeline::sizeHint() const
{
    if (!defined)
    {
        return QSize(32,32);
    }
    return frameSize;
}

void Timeline::resizeEvent(QResizeEvent * /* event */)
{
    refreshPixmap();
}

void Timeline::keyPressEvent(QKeyEvent *event)
{
    //qDebug() << event->key();
    switch(event->key()) {
    case 61:
        zoomFactor += 0.03125;
        zoomFactor = fminf(fmaxf(zoomFactor, 1.0f), 8.0f);
        refreshPixmap();
        update();
        break;

    case 45:
        zoomFactor -= 0.03125;
        zoomFactor = fminf(fmaxf(zoomFactor, 1.0f), 8.0f);
        refreshPixmap();
        update();
        break;
    }
}

void Timeline::wheelEvent(QWheelEvent *event)
{
    int dY = event->angleDelta().y() / 4.0f;
    int dX = event->angleDelta().x() / 4.0f;
    zoomFactor += (1.0f/128.0f) * dY;
    scrollPos -= (1.0f/128.0f) * zoomFactor * dX;

    scrollPos = fminf(fmaxf(scrollPos, 0.0f), thumbnails.size() - 1);
    zoomFactor = fminf(fmaxf(zoomFactor, 1.0f), 8.0f);

    refreshPixmap();

    event->ignore();
}

void Timeline::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseLeftPressed = true;
        mouseLeftDownPos = event->pos();

    } else if (event->button() == Qt::RightButton) {
        mouseRightPressed = true;
        rubberBandShown = true;
        mouseRightDownPos = event->pos();
    }
    mouseLastPosition = event->pos();
}

void Timeline::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseLeftPressed = false;
    } else if (event->button() == Qt::RightButton) {
        mouseRightPressed = false;
        rubberBandShown = false;
        updateRubberBandRegion();
    }
    mouseLastPosition = event->pos();
}

void Timeline::mouseMoveEvent(QMouseEvent *event)
{
    if (mouseLeftPressed) {
        // Perform scrolling/scrubbing
        int dX = event->pos().x() - mouseLastPosition.x();
        scrollPos -= (dX * zoomFactor) / frameSize.width();
        scrollPos = fminf(fmaxf(scrollPos, 0.0f), thumbnails.size() - 1);
        refreshPixmap();
        update();
    }

    if (mouseRightPressed) {
        // Perform rubberband operations
        rubberBandRect = QRect(QPoint(mouseRightDownPos.x(), 0),
                               QPoint(event->pos().x(), frameSize.height()));
        updateRubberBandRegion();
    }
    mouseLastPosition = event->pos();
}


void Timeline::updateRubberBandRegion() {
    QRect rect = rubberBandRect.normalized();
    update(rect);
    // TODO: update where the rect was, not where it is

    /* update(rect.left(), rect.top(), rect.width(), 1);
    update(rect.left(), rect.top(), 1, rect.height());
    update(rect.left(), rect.bottom(), rect.width(), 1);
    update(rect.right(), rect.top(), 1, rect.height()); */
}

/**
 * @brief Timeline::refreshPixmap
 * used for double buffering
 */
void Timeline::refreshPixmap()
{
    pixmap = QPixmap(size());
    pixmap.fill(backgroundColor);

    QPainter painter(&pixmap);
    //painter.begin(this);

    drawThumbnails(&painter);


    update();
}

// Conversion from widget space to frame space
float Timeline::getFrameIndex(const int &x, const int &y) {
    float zf, zf_i, zf_f;
    zf = extractZoomInfo(zoomFactor, zf_i, zf_f);


}


/**
 * @brief Timeline::getFrameIndex
 * @param x_f   Frame space: x position
 * @param zf_i  Full frame interval
 * @param zf_f
 * @return
 */
float Timeline::getFrameIndex(const float &x_f,
                              const float &zf_i,
                              const float &zf_f,
                              bool &halfFrame) {

    float r = fmodf(x_f, zf_i);
    int index = floor(x_f / zf_i) * zf_i;

    if (r < zf_i - zf_f || zf_i < 2) {
        halfFrame = false;
        return index + r / (zf_i - zf_f);
    } else {
        halfFrame = true;
        return index + (zf_i * 0.5f) + (r - zf_i + zf_f) / zf_f;
    }

}


/**
 * @brief Timeline::extractZoomInfo Extracts full frame interval
 *
 * @param zf_i  Frame space: full frame interval
 * @param zf_f  Fractional frame width
 * @return zf
 */
float Timeline::extractZoomInfo(const float &zoomFactor,
                                float &zf_i, float &zf_f) {

    zf_f = zoomFactor - (int)zoomFactor;
    zf_i = (1 << (int)ceil(zoomFactor - 1));

    if (zf_f) {
        // Reverse fractional part of zoomFactor
        zf_f = 1 - zf_f;
    }

    return zf_i + zf_f;
}

/**
 * @brief Timeline::drawThumbnails
 * @param painter
 *
 * @note Implementation choice: can redraw whole pixmap on any change, or only
 * the required segment every time it is required.
 *
 *
 */
void Timeline::drawThumbnails(QPainter *painter)
{
    if (frameSize.width() < 1 || zoomFactor < 1.0f) {
        QString error = QString("Bad render settings. {fw: %1, zf: %2}")
                                .arg(frameSize.width()).arg(zoomFactor);
        qDebug() << error;
        return;
    }

    int ww = size().width();        // widget width
    int fw = frameSize.width();     // frame width
    int fh = frameSize.height();    // frame height

    int selectedFrame = 4;

    bool fracFrames = true; // whether we are displaying fractional frames
    bool isNextFrac = true; // whether next frame is fractional or full

    float zf, zf_i, zf_f;
    zf = extractZoomInfo(zoomFactor, zf_i, zf_f);

    float ffw; // width of fractional frames in widget space

    if ((ffw = (zf_f * fw)) < 0.5f){
        // Fractional frame width is less than one pixel; don't draw frac frames
        zf_f = 0;
        zf = zf_i;

        fracFrames = false;
    }

    if (zf_i == 0) {
        //TODO: test whether we don't need this clause anymore
        qDebug() << "zf_i is ZERO! settings zf_i = 1";
        zf_i++;
    }

    zf = zf_i + zf_f;


    //qDebug() << zoomFactor << " " << zf_i << "\t" << zf_f << "\t" << iFrameDelta;

    QRect srcRect, dstRect;

    float fStart_f;     // frame space: position of next frame to be drawn
    int fStart_w = 0;   // widget space: position of next frame to be drawn
    float iFrame = 0;       // index of next frame to be drawn

    // Cap scroll position so we don't try to render frame at index < 0
    scrollPos = fmaxf(scrollPos, 0.0f);

    // Which frames are rendered in full and which fractionally, depends on zf
    // FULL         FRAC        FULL            FRAC            FULL
    // [0, zf_i)    [zf_i, zf)  [zf, zf+zf_i)   [zf+zf_i, 2zf)  [2zf, 2zf+zf_i)
    // frm 0                    frm ceil(zf)                    frm ceil(2zf)

    fStart_f = scrollPos;
    fStart_w = 0;

    iFrame = getFrameIndex(fStart_f, zf_i, zf_f, isNextFrac);

    int imgIndex;

    while (fStart_w < ww && (ulong)iFrame < thumbnails.size()) {
        QImage image;
        // offset will be 0 apart from first frame which may need to be
        // rendered offscreen
        // TODO: clean this up.
        float offset = iFrame - floor(iFrame);

        if (!(fracFrames && isNextFrac)) {
            // Full frame

            srcRect = QRect(0, 0, fw, fh);
            dstRect = QRect(fStart_w - offset * fw, 0, fw, fh);

            imgIndex = floor(iFrame);

            fStart_w += (1 - offset) * fw;

        } else {
            // Fractional frame
            srcRect = QRect((fw - ffw) * 0.5f, 0, ffw, fh);
            dstRect = QRect(fStart_w - offset * ffw, 0, ffw, fh);

            imgIndex = floor(iFrame);

            fStart_w += (1 - offset) * ffw;
        }

        if ((ulong)imgIndex >= thumbnails.size()) {
            QString error = QString("Cannot access frame {Index: %1, Size: %2}")
                                    .arg(imgIndex).arg(thumbnails.size());
            qDebug() << error;
            return;
        }

        iFrame = floor(iFrame);

        image = thumbnails.at(imgIndex);
        painter->drawImage(dstRect, image, srcRect);
        painter->setPen(foregroundColor);
        if (imgIndex == selectedFrame) {
            painter->drawRect(dstRect.left(), dstRect.top(),
                              dstRect.width() - 1, dstRect.height() - 1);
        }
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(imgIndex)
                          + QString(isNextFrac && fracFrames ? "H" : "F"));

        if (fracFrames) {
            iFrame += zf_i / 2;
            isNextFrac = !isNextFrac;
        } else {
            iFrame += zf_i;
        }

    }

}


void Timeline::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    painter.drawPixmap(0, 0, pixmap);

    if (rubberBandShown) {
        painter.setPen(foregroundColor);
        painter.drawRect(rubberBandRect);
    }

    //painter.drawImage(QRect(QPoint(0, 0), size()), image,
    //                  QRect(QPoint(0, 0), size()));
}
