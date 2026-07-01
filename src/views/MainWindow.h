#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include "api/ApiClient.h"

class TendersView;
class EmailsView;
class SettingsView;
class RagView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void setupConnections();
    
    ApiClient* m_api;
    QTabWidget* m_tabs;
    TendersView* m_tendersView;
    EmailsView* m_emailsView;
    SettingsView* m_settingsView;
	RagView* m_ragView;
};
