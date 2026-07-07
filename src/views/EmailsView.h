#pragma once

#include <QWidget>
#include <QSplitter>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include "widgets/EmailTextBrowser.h"

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
    
    // Пагинация
    void onPrevPage();
    void onNextPage();
    void onPerPageChanged();

private:
    void setupUi();
    void setupConnections();
    void loadCurrentPage();
    void updatePaginationUI(int total);
    void updatePreview(const dto::Email& email);

    ApiClient* m_api;
    EmailsModel* m_model;
    
    // Splitter и панели
    QSplitter* m_splitter;
    
    // Левая панель — фильтры и таблица
    QComboBox* m_categoryFilter;
    QLineEdit* m_searchEdit;
    QPushButton* m_btnRefresh;
    QTableView* m_table;
    QLabel* m_lblTotal;
    QPushButton* m_btnLink;
    
    // Правая панель — предпросмотр
    QLabel* m_lblSubject;
    QLabel* m_lblFrom;
    QLabel* m_lblDate;
    QLabel* m_lblCategory;
    QLabel* m_lblAttachments;
    QLabel* m_lblTenderDetails;
    QTextEdit* m_txtSummary;
    EmailTextBrowser* m_txtBody;
    
    // Пагинация
    QComboBox* m_comboPerPage;
    QPushButton* m_btnPrev;
    QPushButton* m_btnNext;
    QLabel* m_lblPageInfo;
    int m_currentPage;
    int m_perPage;
};