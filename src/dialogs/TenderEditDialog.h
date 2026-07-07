#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QJsonObject>

#include "api/ApiClient.h"
#include "api/DtoModels.h"

class TenderEditDialog : public QDialog {
    Q_OBJECT

public:
    // Новый тендер (пустой)
    explicit TenderEditDialog(ApiClient* api, QWidget* parent = nullptr);
    
    // Редактирование существующего тендера
    explicit TenderEditDialog(ApiClient* api, QWidget* parent, int tenderId);
    
    // Создание тендера из данных письма (LLM)
    explicit TenderEditDialog(ApiClient* api, QWidget* parent, const QJsonObject& tenderDetails);
    
    // Получить данные тендера после accept()
    dto::Tender getTender() const;
	
	// Получить ID созданного тендера (после accept())
	int getCreatedTenderId() const { return m_createdTenderId; }
    
    // Получить список прикреплённых файлов (полные пути)
    QStringList getAttachedFiles() const;

private slots:
    void onAccept();
    void onAddFiles();
    void onRemoveFile();

private:
    void setupUi();
    void loadTender(int id);
    
    ApiClient* m_api;
    int m_tenderId = 0;
	int m_createdTenderId = 0;
    bool m_isNew = true;
    
    // Поля формы
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
    
    // Прикреплённые файлы
    QListWidget* m_listFiles;
    QPushButton* m_btnAddFiles;
    QPushButton* m_btnRemoveFile;
    QStringList m_attachedFiles;
};