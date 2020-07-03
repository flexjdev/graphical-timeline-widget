#include <QApplication>

#include "timeline.h"

#include <QDebug>
#include <QString>

class Timeline;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Timeline timeline;

    QString filename = QString(":/images/f001.png");
    QImage earth(filename);
    earth = earth.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    timeline.init(45, earth.size());
    timeline.show();



    for (int i = 0; i < 45; i++) {
        filename = QString(":/images/f%1.png").arg(i, 3, 10, QChar('0'));
        earth = QImage(filename);
        earth = earth.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        timeline.setImage(i, earth);
    }





    return app.exec();
}
