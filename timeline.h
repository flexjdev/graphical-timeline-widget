#ifndef ICONEDITOR_H
#define ICONEDITOR_H

#include <QColor>
#include <QImage>
#include <QWidget>
#include <vector>
#include <stack>


/**
 * Custom Widget that displays a timeline composed of thumbnails.
 *
 * @author Chris Janusiewicz
 *
 */
class Timeline : public QWidget
{
    Q_OBJECT

public:
    Timeline(QWidget *parent = 0);

    void init(int frameCount, QSize frameSize);
    void insertImage(int index, const QImage &image);
    void setImage(int index, const QImage &image);
    void updateImage(int index, const QImage &image);
    void removeImage(int index);

    void setZoom(const float &z);
    void setPos(const float &z);
    void adjustZoom(const int &px);
    void adjustPos(const int &px);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    bool isDefined() { return defined; }

public slots:
    void setSelectIndex(const int &index);

signals:
    void onFrameClick(int frameid);
    void selectIndexChanged(int selectIndex);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent* event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    bool defined;

    QColor backgroundColor;
    QColor foregroundColor;
    QImage image;
    QSize frameSize;    // size of each thumbnail must stay the same across gif

    int selectIndex, hoverIndex;
    QRect selectRect, hoverRect;
    bool hoverChanged, selectChanged;  // Must update() if either is true

    bool mouseLeftPressed, mouseRightPressed;
    QPoint mouseLeftDownPos, mouseRightDownPos;
    QPoint mouseLastPosition;

    bool rubberBandShown;
    QRect rubberBandRect;
    QPixmap pixmap;

    // TODO: encapsulate zoom/scroll settings in struct
    float scrollPos, sp_f;
    float zoomFactor, zf_f, zf_i;
    bool halfFrames;

    // TODO: store delays
    std::vector<QImage> thumbnails; // Store thumbnails in the timeline struct

    bool checkSize(const QImage &image);

    QRect getFrameRect(const int &x_w); // Returns bounds of frame under cursor
    float getFrameIndex(const float &x_f, bool &halfFrame);
    int getFullFrameIndex(const float &x_f);

    float toFrameSpace(const int &x_w);
    int toWidgetSpace(const float &x_f);

    void refreshPixmap();
    void drawThumbnails(QPainter *painter);

    void updateRubberBandRegion();
    void updateOverlay();
};

#endif
