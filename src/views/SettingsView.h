#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QCheckBox>
#include "api/ApiClient.h"

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(ApiClient* api, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onTestConnection();
    void onProcessNow();
    void onAutoRefreshToggled(bool checked);
    void onRefreshStatistics();

private:
    void setupUi();
    void setupConnections();
    void loadStatistics();
    void updateConnectionStatus(bool connected, const QString& message);

    ApiClient* m_api;

    // Подключение
    QLineEdit* m_editApiUrl;
    QPushButton* m_btnTestConnection;
    QLabel* m_lblConnectionStatus;

    // Обработка почты
    QSpinBox* m_spinInterval;
    QPushButton* m_btnProcessNow;
    QCheckBox* m_chkAutoRefresh;
    QLabel* m_lblProcessStatus;

    // Статистика
    QLabel* m_lblTotalEmails;
    QLabel* m_lblTenderEmails;
    QLabel* m_lblSpamEmails;
    QLabel* m_lblTotalTenders;
    QLabel* m_lblTotalDocuments;
    QLabel* m_lblIndexedDocuments;
    QLabel* m_lblTotalChunks;
    QLabel* m_lblRagQueries;

    // Таймер автообновления
    QTimer* m_refreshTimer;
};
