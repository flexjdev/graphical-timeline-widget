#include <QtWidgets>

#include "timeline.h"
#include <QDebug>
#include <QPoint>
#include <QRect>


#define DEFAULT_ZOOM_INTERVAL 1.0f / 32.0f


// TODO: Alter scrolling such that scrolling across H frames is not zoomfactor
// times faster than scrolling across F frames

// TODO: Implement zooming centered around a selected frame/middle frame

// TODO: rename fractional/frac frames to (H)alf frames for consistency

// TODO: Find correct Qt::Keys for - and + for zooming

// TODO: change underlying storage from vector to something more appropriate:
// consider using linked list: frames may be inserted and deleted in the middle,
// whereas unlikely to be added to the end. Random access, indexing? Most likely
// is not a big issue seeing as this is for manual editing of frames, projects
// most likely not going to be bigger than 30 * 60 * 10 = 18,000 frames at most

Timeline::Timeline(QWidget *parent) : QWidget(parent)
{
    defined = false;
    setAttribute(Qt::WA_StaticContents);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    backgroundColor = QWidget::palette().color(QWidget::backgroundRole());
    foregroundColor = QWidget::palette().color(QWidget::foregroundRole());

    // enable mouse move events even when no buttons are pressed
    // TODO: find out if its possible to stop paint events being fired every
    // time the mouse moves across the widget
    // TODO: find out why mouse behaves as if captured, on startup
    setMouseTracking(true);

    rubberBandShown = false;
    image = QImage(size(), QImage::Format_ARGB32);
    image.fill(Qt::black);

    zoomFactor = 1.5f;
    scrollPos = 0.0f;

    indexSelected = -1;
    indexHover = -1;

    refreshPixmap();
}

/**
 * @brief Timeline::init Initialises the widget with a given number of frames;
 * theses start off as empty so be sure to update their thumbnails.
 * @param frameCount
 * @param frameSize The size in pixels of the thumbnails, must stay the same
 * across every frame: this widget does not perform any resizing on its images
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
    return QSize(frameSize.width() * fmin(thumbnails.size(), 8),
                 frameSize.height());
}

QSize Timeline::minimumSizeHint() const
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
    switch(event->key()) {
    case Qt::Key_Minus:
        zoomContent(DEFAULT_ZOOM_INTERVAL);
        break;

    case Qt::Key_Equal:
        zoomContent(-DEFAULT_ZOOM_INTERVAL);
        break;
    }
}

void Timeline::wheelEvent(QWheelEvent *event)
{
    int dY = event->pixelDelta().y();   // Scrolling up and down zooms
    int dX = event->pixelDelta().x();   // Scrolling left and right scrolls

    // Do not allow the user to scroll horizontally & vertically simultaneously
    if (abs(dY) > abs(dX)) {
        zoomContent(dY);
    } else {
         scrollContent(dX);
    }

    event->ignore();
}

void Timeline::scrollContent(const int &px) {
    scrollPos -= (px * zoomFactor) / frameSize.width();
    scrollPos = fminf(fmaxf(scrollPos, 0.0f), thumbnails.size() - 1);

    refreshPixmap();
    update();
}

void Timeline::zoomContent(const int &px) {
    zoomFactor += (1.0f/128.0f) * px;
    zoomFactor = fminf(fmaxf(zoomFactor, 1.0f), 8.0f);

    refreshPixmap();
    update();
}

void Timeline::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseLeftPressed = true;
        mouseLeftDownPos = event->pos();

        setCursor(QCursor(Qt::ClosedHandCursor));

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

        setCursor(QCursor(Qt::ArrowCursor));

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
        scrollContent(dX);
    }

    if (mouseRightPressed) {
        // Perform rubberband operations
        rubberBandRect = QRect(QPoint(mouseRightDownPos.x(), 0),
                               QPoint(event->pos().x(), frameSize.height()));
        updateRubberBandRegion();
    }

    if (hoverChanged(event->pos())) {
        update();
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

    drawThumbnails(&painter);

    update();
}


bool Timeline::hoverChanged(const QPoint &mousePos)
{
    // get floating point index under cursor
    int index = (int)getFrameIndex(mousePos.x(), mousePos.y());

    if (index != indexHover) {
        // if index changed, repaint
        indexHover = index;
        return true;
    }

    return false;
}


QRect Timeline::getFrameRect(const int &index)
{
    int left, width;
    float zf_i, zf_f;
    extractZoomInfo(zoomFactor, zf_i, zf_f);

    bool hFrame = !((index % (int)zf_i) == 0.0f);

    float offset = (index - scrollPos) * (1.0f + zf_f) / zf_i;

    if (hFrame) {
        offset += 1 / (zf_f + 1);
        width = zf_f * frameSize.width();
    } else {
        width = frameSize.width();
    }
    //qDebug() << hFrame << " " << index << " " << offset;

    left = offset * frameSize.width();

    return QRect(QPoint(left, 0), QSize(width, frameSize.height()));
}


// Conversion from widget space to frame space
float Timeline::getFrameIndex(const int &x_w, const int &y_w)
{
    if (y_w < 0 && y_w > frameSize.height()) {
        return -1;
    }

    float zf, zf_i, zf_f;
    zf = extractZoomInfo(zoomFactor, zf_i, zf_f);

    int fw = frameSize.width();

    float x_f = scrollPos + ((float)x_w / (fw * (1 + zf_f))) * zf_i;
    bool b;
    float index = getFrameIndex(x_f, zf_i, zf_f, b);
    qDebug() << (b ? "H" : "F") << " " << x_w << " " << x_f << " " << index << " " << zf_i << " " <<  zf_f;
    return index;
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
    int index = floor(x_f / zf_i) * zf_i;   // Integer part of index

    //r *= (1.0f + zf_f);

    if (r / zf_i < 1 / (zf_f + 1) || zf_i < 2) {
        halfFrame = false;
        //qDebug() << (halfFrame ? "H" : "F") << " " << r << " " << r / zf_i << " " << 1 / (zf_f + 1) << " " << index << " " << zf_i << " " <<  zf_f;
        return index + r / (zf_i - zf_f);
    } else {
        halfFrame = true;
        //qDebug() << (halfFrame ? "H" : "F") << " " << r << " " << r / zf_i << " " << 1 / (zf_f + 1) << " " << index << " " << zf_i << " " <<  zf_f;
        return index + (zf_i * 0.5f) + (r - zf_i + zf_f) / zf_f;
    }

}


/**
 * @brief Timeline::extractZoomInfo Converts linearly increasing zoomfactor
 * to
 * @param zoomFactor
 * @param zf_i  Frame space: full frame interval
 * @param zf_f  Half frame width
 * @return
 */
float Timeline::extractZoomInfo(const float &zoomFactor,
                                float &zf_i,
                                float &zf_f)
{

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

    QRect srcRect, dstRect;

    float fStart_f;     // frame space: position of next frame to be drawn
    int fStart_w = 0;   // widget space: position of next frame to be drawn
    float iFrame = 0;       // index of next frame to be drawn

    // Cap scroll position so we don't try to render frame at index < 0
    fStart_f = fmaxf(scrollPos, 0.0f);
    fStart_w = 0;

    // Which frames are rendered in full and which fractionally, depends on zf
    // FULL         FRAC        FULL            FRAC            FULL
    // [0, zf_i)    [zf_i, zf)  [zf, zf+zf_i)   [zf+zf_i, 2zf)  [2zf, 2zf+zf_i)
    // frm 0                    frm ceil(zf)                    frm ceil(2zf)
    iFrame = getFrameIndex(fStart_f, zf_i, zf_f, isNextFrac);

    int imgIndex;

    painter->setPen(foregroundColor);

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

        if (imgIndex < 0 || (ulong)imgIndex >= thumbnails.size()) {
            QString error = QString("Cannot access frame {Index: %1, Size: %2}")
                                    .arg(imgIndex).arg(thumbnails.size());
            qDebug() << error;
            return;
        }

        iFrame = floor(iFrame);

        image = thumbnails.at(imgIndex);
        painter->drawImage(dstRect, image, srcRect);

        // Debugging: draw frame index and (F)ull or (H)alf identified
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(imgIndex)
                          + QString(isNextFrac && fracFrames ? "H" : "F"));

        // Advance frame index to the next frame
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

    painter.setPen(foregroundColor);
    painter.setPen(Qt::magenta); // easier to see debug text

    if (rubberBandShown) {
        painter.drawRect(rubberBandRect);
    }

    // testing painting index;
    if (indexHover != -1) {
        QRect hover = getFrameRect(indexHover);
        painter.drawRect(hover);
        painter.drawText(hover.center(), QString::number(indexHover));
    }


}
