// LinkEmailDialog.h
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include "api/ApiClient.h"
#include "api/DtoModels.h"

class LinkEmailDialog : public QDialog {
    Q_OBJECT
public:
    explicit LinkEmailDialog(ApiClient* api, const dto::Email& email, QWidget* parent = nullptr);

    int selectedTenderId() const { return m_selectedTenderId; }
    int getCreatedTenderId() const { return m_createdTenderId; }

private slots:
    void onTenderSelected(int index);
    void onCreateNewTender();
    void onLink();

private:
    void setupUi();
    void loadTenders();

    ApiClient* m_api;
    dto::Email m_email;
    int m_selectedTenderId = 0;
	int m_createdTenderId = 0;

    QLabel* m_lblEmailInfo;
    QComboBox* m_comboTenders;
    QPushButton* m_btnCreateNew;
    QPushButton* m_btnLink;
    QList<dto::Tender> m_tenders;
};