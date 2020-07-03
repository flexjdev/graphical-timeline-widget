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
        return;
    }

    bool fracFrames = true; // whether we are displaying fractional frames
    bool isNextFrac;    // whether next frame is fractional or full

    float zf;
    float zf_f = zoomFactor - (int)zoomFactor;
    float zf_i = (1 << (int)ceil(zoomFactor - 1)) - 1;


    int iFrameDelta; // how many images to advance between full frame
    int iFracFrameDelta;  // offset from full to frac frame

    if (zf_f) {
        // Reverse fractional part of zoomFactor
        zf_f = 1 - zf_f;
    } else {
        // Fractional part is zero; only drawing full frames.
        fracFrames = false;
    }
    iFrameDelta = floor(zf_i) +1;
    iFracFrameDelta = iFrameDelta / 2;

    if (zf_i == 0) {
        zf_i++;
        iFrameDelta = 1;
    }

    zf = zf_i + zf_f;

    int ww = size().width();    // widget width
    int fw = frameSize.width(); // frame width
    int fh = frameSize.height();
    float ffw = zf_f * fw; // width of fractional frames in widget space

    //qDebug() << zoomFactor << " " << zf_i << "\t" << zf_f << "\t" << iFrameDelta;

    QRect srcRect, dstRect;

    float fStart_f;     // frame space: position of next frame to be drawn
    int fStart_w = 0;   // widget space: position of next frame to be drawn
    int iFrame, iFracFrame;     // index of frame and frac frame to be drawn
    //bool isFull;    // whether next frame is full of fractional

    // Cap scroll position so we don't try to render frame at index < 0
    scrollPos = fmaxf(scrollPos, 0.0f);

    // Which frames are rendered in full and which fractionally, depends on zf
    // FULL         FRAC        FULL            FRAC            FULL
    // [0, zf_i)    [zf_i, zf)  [zf, zf+zf_i)   [zf+zf_i, 2zf)  [2zf, 2zf+zf_i)
    // frm 0                    frm ceil(zf)                    frm ceil(2zf)
    fStart_w = 0;
    fStart_f = scrollPos;

    // get index of first frame drawn
    iFrame = (zf_i + 1) * floor(fStart_f / zf);
    iFracFrame = iFrame + iFracFrameDelta;

    // must get remainder of dividing by zf to determine whether first frame is
    // full or fractional
    float r = fmodf(fStart_f, zf);

    // Part of the first frame might be hidden off screen, only remainder needs
    // to be drawn: this is a special case


    if (r < zf_i || !fracFrames) {
        // draw full frame
        float offset = fw * (r / zf_i); // how much of frame is offscreen
        float width = fw - offset; // how much of frame must be drawn

        srcRect = QRect(offset, 0, width, fh);
        dstRect = QRect(fStart_w, 0, width, fh);

        if ((ulong)iFrame >= thumbnails.size()) {
            qDebug() << "Tried to access past end of vector on first frame";
            return;
        }
        QImage image = thumbnails.at(iFrame);
        painter->drawImage(dstRect, image, srcRect);
        painter->setPen(foregroundColor);
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(iFrame) + QString("F"));


        fStart_w += width;

        isNextFrac = true;
    } else {
        // draw fractional frame
        r -= zf_i;  // take away integer part of remainder
        float offset = (r / zf_f); // how much of frame is offscreen
        float drawn = 1 - offset; // how much of frame must be drawn

        // Use half of offset while drawing from source image to center it
        srcRect = QRect(ffw * offset * 0.5f, fh * offset * 0.5f,
                        drawn * ffw, fh * drawn);
        dstRect = QRect(fStart_w, fh * offset,
                        drawn * ffw, fh * drawn);

        if ((ulong)iFracFrame >= thumbnails.size()) {
            qDebug() << "Tried to access past end of vector on first frame";
            return;
        }
        QImage image = thumbnails.at(iFracFrame);
        painter->drawImage(dstRect, image, srcRect);
        painter->setPen(foregroundColor);
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(iFracFrame) + QString("H"));


        fStart_w += drawn * ffw;
        isNextFrac = false;
    }
    iFrame += iFrameDelta;
    iFracFrame +=iFracFrameDelta;

    // Now draw the rest of the frames, fully
    while (fStart_w < ww && (ulong)iFrame < thumbnails.size()) {
        QImage image;
        int imgIndex;

        if (!(fracFrames && isNextFrac)) {
            // Full frame
            srcRect = QRect(0, 0, fw, fh);
            dstRect = QRect(fStart_w, 0, fw, fh);

            imgIndex = iFrame;

            fStart_w += fw;
            iFrame += iFrameDelta;
            iFracFrame = iFrame + iFracFrameDelta;

        } else {
            // Fractional frame
            srcRect = QRect((fw - ffw) * 0.5f, 0, ffw, fh);
            dstRect = QRect(fStart_w, 0, ffw, fh);

            imgIndex = iFracFrame;
            fStart_w += ffw;
        }

        if ((ulong)imgIndex >= thumbnails.size()) {
            qDebug() << "Tried to access past end of vector";
            return;
        }

        image = thumbnails.at(imgIndex);
        painter->drawImage(dstRect, image, srcRect);
        painter->setPen(foregroundColor);
        painter->drawText(dstRect, Qt::AlignHCenter | Qt::AlignTop,
                          QString::number(imgIndex) + QString(isNextFrac && fracFrames ? "H" : "F"));

        if (fracFrames) {
            isNextFrac = !isNextFrac;
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
