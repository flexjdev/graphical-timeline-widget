#include <QApplication>

#include "timeline.h"

#include <QDebug>
#include <QString>

class Timeline;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Timeline timeline;
    QSize thumbnailSize(128, 128);  // Define size of thumbnails in widget

    // Initlialise timeline to 45 empty images and thumbnail size of 128x128
    timeline.init(45, thumbnailSize);

    // Timeline can be shown before defining images
    // Images can be buffered in one at a time
    timeline.show();


    QString filename;
    QImage img;
    // Fill timeline with images. Can be done as needed, or in advance
    for (int i = 0; i < 45; i++) {
        // Test images stored in Qt resources
        // Generate filename of the form f###.png
        filename = QString(":/images/f%1.png").arg(i, 3, 10, QChar('0'));

        // Load image. Must scale it to appropriate size for the timeline.
        img = QImage(filename).scaled(thumbnailSize,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation);

        // Supply image to timeline
        timeline.setImage(i, img);
    }

    return app.exec();
}
