#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QComboBox>
#include "api/ApiClient.h"

class TenderEditDialog : public QDialog {
    Q_OBJECT
public:
    // Создание нового тендера
    explicit TenderEditDialog(ApiClient* api, QWidget* parent = nullptr);
    // Редактирование существующего
    explicit TenderEditDialog(ApiClient* api, QWidget* parent, int tenderId);
	// Создание тендера с предзаполненными данными из письма
    explicit TenderEditDialog(ApiClient* api, QWidget* parent, const QJsonObject& tenderDetails);

private slots:
    void onAccept();

private:
    void setupUi();
    void loadTender(int id);
    
    ApiClient* m_api;
    int m_tenderId = 0;
    bool m_isNew = true;
    
    QLineEdit* m_editNoticeNumber;
    QLineEdit* m_editLotNumber;
    QLineEdit* m_editPurchaseName;
    QLineEdit* m_editCustomer;
    QLineEdit* m_editEtpUrl;
    QDoubleSpinBox* m_spinNmck;
    QComboBox* m_comboCurrency;
    QDateTimeEdit* m_dtDeadline;
    QComboBox* m_comboStatus;
    QTextEdit* m_editNotes;
};