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

// TODO: double buffering

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

    QSize sizeHint() const;

    bool isDefined() { return defined; }

public slots:

signals:
    void onFrameClick(int frameid);

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


    bool mouseLeftPressed, mouseRightPressed;
    QPoint mouseLeftDownPos, mouseRightDownPos;
    QPoint mouseLastPosition;

    bool rubberBandShown;
    QRect rubberBandRect;
    QPixmap pixmap;

    float zoomFactor;
    float scrollPos;


    std::vector<QImage> thumbnails; // Store thumbnails in the timeline struct


    // TODO: store delays

    bool checkSize(const QImage &image);
    void updateRubberBandRegion();
    float getFrameIndex(const int &x_w, const int &y_w);
    float getFrameIndex(const float &x_f, const float &zf_i, const float &zf_f,
                        bool &halfFrame);
    float extractZoomInfo(const float &zoomFactor, float &zf_i, float &zf_f);

    void refreshPixmap();
    void drawThumbnails(QPainter *painter);

};

#endif
