#pragma once
#include <QWidget>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "api/ApiClient.h"
#include "models/TendersModel.h"

class TendersView : public QWidget {
    Q_OBJECT
public:
    explicit TendersView(ApiClient* api, QWidget* parent = nullptr);
    
    void refresh();

private slots:
    void onAddTender();
    void onEditTender();
    void onDeleteTender();
    void onRowDoubleClicked(const QModelIndex& index);
    void onSearch();
    void onStatusFilterChanged();
    void onPageChanged();

private:
    void setupUi();
    void setupConnections();
    
    ApiClient* m_api;
    TendersModel* m_model;
    
    QTableView* m_table;
    QLineEdit* m_searchEdit;
    QComboBox* m_statusFilter;
    QPushButton* m_btnAdd;
    QPushButton* m_btnEdit;
    QPushButton* m_btnDelete;
    QPushButton* m_btnRefresh;
    QLabel* m_lblTotal;
};