#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QDateEdit>
#include <QCheckBox>
#include "api/ApiClient.h"

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(ApiClient* api, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onCheckImap();
    void onTestConnection();
    void onProcessNow();
    void onAutoRefreshToggled(bool checked);
    void onRefreshStatistics();
	void onProcessModeChanged();
	

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
	
	// Режим обработки
    QRadioButton* m_radioLastN;
    QRadioButton* m_radioSinceDate;
    QSpinBox* m_spinLimit;
    QComboBox* m_comboPeriod;
    QDateEdit* m_dateEdit;

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
	
	// Проверка IMAP
    QGroupBox* m_imapStatusGroup;
    QLabel* m_lblImapStatus;
    QLabel* m_lblImapDetails;
    QPushButton* m_btnCheckImap;
    QWidget* m_imapStepsWidget;
    QVBoxLayout* m_imapStepsLayout;
};
