#pragma once
#include <QWidget>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QTextEdit>
#include "api/ApiClient.h"
#include "models/EmailsModel.h"

class EmailsView : public QWidget {
    Q_OBJECT
public:
    explicit EmailsView(ApiClient* api, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onRowClicked(const QModelIndex& index);
    void onRowDoubleClicked(const QModelIndex& index);
    void onSearch();
    void onCategoryFilterChanged();
    void onLinkToTender();

private:
    void setupUi();
    void setupConnections();
    void updatePreview(const dto::Email& email);
    
    ApiClient* m_api;
    EmailsModel* m_model;
    
    // Левая панель — список писем
    QTableView* m_table;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryFilter;
    QPushButton* m_btnRefresh;
    QPushButton* m_btnLink;
    QLabel* m_lblTotal;
    
    // Правая панель — предпросмотр
    QSplitter* m_splitter;
    QWidget* m_previewWidget;
    QLabel* m_lblSubject;
    QLabel* m_lblFrom;
    QLabel* m_lblDate;
    QLabel* m_lblCategory;
    QTextEdit* m_txtSummary;
    QTextEdit* m_txtBody;
    QLabel* m_lblAttachments;
    QLabel* m_lblTenderDetails;
};