#include <QApplication>
#include <QSqlDatabase>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Устанавливаем стиль приложения
    app.setStyle("Fusion");

    // Создаем и показываем главное окно
    MainWindow window;
    window.show();

    return app.exec();
}
