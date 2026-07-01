#include <QApplication>
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("TenderCrmDesktop");
    app.setOrganizationName("Sevisu");
    app.setApplicationVersion("1.0");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}