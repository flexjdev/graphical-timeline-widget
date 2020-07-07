#include <QtWidgets>

#include "timeline.h"
#include <QDebug>
#include <QPoint>
#include <QRect>

#define DEFAULT_ZOOM_INTERVAL 1.0f / 32.0f
#define DEFAULT_SCROLL_INTERVAL 1.0f / 32.0f


Timeline::Timeline(QWidget *parent) : QWidget(parent)
{
    defined = false;
    setAttribute(Qt::WA_StaticContents);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    //setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    backgroundColor = QWidget::palette().color(QWidget::backgroundRole());
    foregroundColor = QWidget::palette().color(QWidget::foregroundRole());

    setMouseTracking(true);

    rubberBandShown = false;
    image = QImage(size(), QImage::Format_ARGB32);
    image.fill(Qt::black);

    frameSize = QSize(32,32);
    setZoom(2.75f);
    setPos(16.0f);

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
        return QSize(32, 32);
    }
    return QSize(frameSize.width() * 8, frameSize.height());
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
    int dY = event->pixelDelta().y();   // Scrolling up and down zooms
    int dX = event->pixelDelta().x();   // Scrolling left and right scrolls

    // Do not allow the user to scroll horizontally & vertically simultaneously
    if (abs(dY) > abs(dX)) {
        adjustZoom(dY);
    } else {
        adjustPos(dX);
    }

    event->ignore();
}

void Timeline::adjustPos(const int &px)
{
    setPos(scrollPos - px * zoomFactor / frameSize.width());
}

void Timeline::setPos(const float &pos)
{
    float minScrollPos = zf_i * width() * 0.5f / ((1.0f + zf_f) * frameSize.width());

    scrollPos = fminf(fmaxf(pos, minScrollPos), thumbnails.size() - 1);

    sp_f = scrollPos;

    refreshPixmap();
    update();
}


void Timeline::setZoom(const float &zoom)
{
    // Maximum zoom N means n=2^(N-1), Every nth frame drawn
    zoomFactor = fminf(fmaxf(zoom, 1.0f), 5.0f);

    zf_f = zoomFactor - (int)zoomFactor; // Width of half frames
    zf_i = (1 << (int)ceil(zoomFactor - 1)); // Frame space: full frame interval

    if (zf_f * frameSize.width() < 0.5f){
        // Fractional frame width is less than one pixel; don't draw frac frames
        zf_f = 0;
        halfFrames = false;
    } else {
        // Reverse fractional part of zoomFactor (for smooth scrolling)
        zf_f = 1 - zf_f;
        halfFrames = true;
    }

    // May need to adjust scroll position if we zoomed out far enough
    adjustPos(0);

    refreshPixmap();
    update();
}

void Timeline::adjustZoom(const int &px) {
    setZoom(zoomFactor + (1.0f/128.0f) * px);
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
        adjustPos(dX);
    }

    if (mouseRightPressed) {
        // Perform rubberband operations
        rubberBandRect = QRect(QPoint(mouseRightDownPos.x(), 0),
                               QPoint(event->pos().x(), frameSize.height()));
        updateRubberBandRegion();
    }

    mouseLastPosition = event->pos();
    update();
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

    drawThumbnails(&painter);

    update();
}

/**
 * @brief Timeline::getFrameAtIndex
 * @param x_f
 * @param halfFrame
 * @return
 */
float Timeline::getFrameIndex(const float &x_f, bool &halfFrame)
{
    // Which frames are rendered in full and which fractionally, depends on zf

    // WIDGET SPACE
    // FULL      FRAC           FULL            FRAC
    // [0, 1)    [1, 1+zf_f)    [1+zf, 2+zf)    [2+2zf, 2+2zf)
    // 1         zf             1               zf

    // FRAME SPACE
    // 0         zf_i / 2       zf_i            zf_i + zf_i / 2

    float r = fmodf(x_f, zf_i);
    int index = floor(x_f / zf_i) * zf_i;   // Integer part of index

    // Boundary between full frame and half frame is at r = zf_i / (1 + zf_f)

    if (r < zf_i / (zf_f + 1) || zf_i < 2) {
        // if zf_i < 2, there are no half frame indices between full frames
        halfFrame = false;
        return index + r * (1 + zf_f) / zf_i;
    } else {
        halfFrame = true;
        //qDebug() << (halfFrame ? "H" : "F") << " " << r << " " << r / zf_i << " " << 1 / (zf_f + 1) << " " << index << " " << zf_i << " " <<  zf_f;
        return index + (zf_i * 0.5f)
                + (r - (zf_i / (1 + zf_f))) / (zf_i - (zf_i / (1 + zf_f)));
    }
}


int Timeline::getFullFrameIndex(const float &x_f)
{
    return floor(x_f / zf_i) * zf_i;   // Integer part of index
}



// Conversion from widget space to frame space
float Timeline::toFrameSpace(const int &x_w)
{
    return sp_f + zf_i *
            (x_w - 0.5f * width()) / ((1.0f + zf_f) * frameSize.width());
}

int Timeline::toWidgetSpace(const float &x_f)
{
    return 0.5f * width() +
            frameSize.width() * (zf_f + 1) * (x_f - sp_f) / zf_i;
}


/**
 * @brief Timeline::getFrameRect Rect of frame drawn under cursor
 * @param x_w
 * @return
 */
QRect Timeline::getFrameRect(const int &x_w)
{
    int left, width;
    width = frameSize.width();

    bool hFrame;

    float fIndex = getFrameIndex(toFrameSpace(x_w), hFrame);

    if (!hFrame) {
        width = frameSize.width();
        left = toWidgetSpace(floor(fIndex));
    } else {
        width = zf_f * frameSize.width();
        left = toWidgetSpace(floor(fIndex - zf_i / 2)) + frameSize.width();
    }
    return QRect(left, 0, width, frameSize.height());
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
    float hfw = fw * zf_f; // width of half frames in widget space

    bool halfFrame = false; // whether next frame is full or half

    float fStart_f;     // frame space: position of next frame to be drawn
    float fStart_w = 0;   // widget space: position of next frame to be drawn
    int iFrame = 0;   // index of next frame to be drawn

    fStart_f = toFrameSpace(0); //

    iFrame = getFullFrameIndex(fStart_f);

    fStart_w = toWidgetSpace(iFrame);

    painter->setPen(foregroundColor);

    QRect srcRect, dstRect;

    while (fStart_w < ww && (ulong)iFrame < thumbnails.size()) {

        if (!(halfFrames && halfFrame)) {
            // Full frame
            srcRect = QRect(0, 0, fw, fh);
            dstRect = QRect(fStart_w, 0, fw, fh);

            fStart_w += fw;

        } else {
            // Half frame
            srcRect = QRect(fw * (1 - zf_f) * 0.5f, 0, hfw, fh);
            dstRect = QRect(fStart_w, 0, hfw, fh);

            fStart_w += hfw;
        }

        if (iFrame < 0 || (ulong)iFrame >= thumbnails.size()) {
            QString error = QString("Cannot access frame {Index: %1, Size: %2}")
                                    .arg(iFrame).arg(thumbnails.size());
            qDebug() << error;
            return;
        }

        QImage image = thumbnails.at(iFrame);

        if (dstRect.right() >= 0) {
            painter->drawImage(dstRect, image, srcRect);
        }


        // Debugging: draw frame index and (F)ull or (H)alf identified
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(iFrame)
                          + QString(halfFrame && halfFrames ? "H" : "F"));

        // Advance frame index to the next frame
        if (halfFrames) {
            iFrame += zf_i / 2;
            halfFrame = !halfFrame;
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
        painter.drawRect(rubberBandRect);
    }

    painter.setPen(Qt::magenta);

    painter.drawText(QPoint(16,16), QString("%1:%2").arg(zf_i).arg(zf_f));

    bool b;
    int m_w = mouseLastPosition.x();
    float m_f = toFrameSpace(m_w);
    float m_i = getFrameIndex(m_f, b);

    painter.drawLine(m_w, 0, m_w, height());
    painter.drawText(QPoint(m_w + 16, 16), QString("i: %1 %2").arg(m_i).arg(b ? "H" : "F"));
    painter.drawText(QPoint(m_w + 16, 32), QString("f: %1").arg(m_f));

    QRect hoverRect = getFrameRect(m_w);

    painter.setPen(foregroundColor);
    painter.drawRect(hoverRect);
    //qDebug() << hoverRect;

    //painter.drawText(QPoint(x_m + 16, 48), QString("%1: %2").arg(m_i).arg(b ? "H" : "F"));


}
