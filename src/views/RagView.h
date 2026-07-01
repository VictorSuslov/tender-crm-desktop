#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QSpinBox>
#include "api/ApiClient.h"

class RagView : public QWidget {
    Q_OBJECT
public:
    explicit RagView(ApiClient* api, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onAskQuestion();
    void onTenderChanged();
    void onHistoryItemClicked(QListWidgetItem* item);
    void onLoadHistory();

private:
    void setupUi();
    void setupConnections();
    void loadTenders();
    void displayAnswer(const QJsonObject& result);
    void displaySources(const QJsonArray& sources);
    
    ApiClient* m_api;
    
    // Верхняя панель: выбор тендера и ввод вопроса
    QComboBox* m_comboTenders;
    QLineEdit* m_editQuestion;
    QPushButton* m_btnAsk;
    QSpinBox* m_spinTopK;
    
    // Центральная область: ответ и источники
    QTextEdit* m_txtAnswer;
    QListWidget* m_lstSources;
    
    // Правая панель: история
    QListWidget* m_lstHistory;
    QPushButton* m_btnRefreshHistory;
    
    // Кэшированные данные
    QList<QPair<int, QString>> m_tenders;  // id, name
};
