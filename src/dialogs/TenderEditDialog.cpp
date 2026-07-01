#include "TenderEditDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLabel>
#include <QRegularExpression>

TenderEditDialog::TenderEditDialog(ApiClient* api, QWidget* parent)
    : QDialog(parent), m_api(api), m_isNew(true)
{
    setupUi();
    setWindowTitle("Новый тендер");
}

TenderEditDialog::TenderEditDialog(ApiClient* api, QWidget* parent, int tenderId)
    : QDialog(parent), m_api(api), m_tenderId(tenderId), m_isNew(false)
{
    setupUi();
    setWindowTitle("Редактирование тендера");
    loadTender(tenderId);
}

TenderEditDialog::TenderEditDialog(ApiClient* api, QWidget* parent, const QJsonObject& tenderDetails)
    : QDialog(parent), m_api(api), m_isNew(true)
{
    setupUi();
    setWindowTitle("Новый тендер (из письма)");
    
    // Предзаполняем из данных LLM
    if (tenderDetails.contains("notice_number") && !tenderDetails["notice_number"].isNull())
        m_editNoticeNumber->setText(tenderDetails["notice_number"].toString());
    
    if (tenderDetails.contains("purchase_name") && !tenderDetails["purchase_name"].isNull())
        m_editPurchaseName->setText(tenderDetails["purchase_name"].toString());
    
    // Парсим НМЦК (может быть строкой "64 833 943,20 руб.")
    if (tenderDetails.contains("nmck") && !tenderDetails["nmck"].isNull()) {
        QString nmckStr = tenderDetails["nmck"].toString();
        // Убираем все кроме цифр и точки
        nmckStr.replace(QRegularExpression("[^0-9.]"), "");
        bool ok;
        double nmck = nmckStr.toDouble(&ok);
        if (ok && nmck > 0)
            m_spinNmck->setValue(nmck);
    }
    
    // Парсим дедлайн
    if (tenderDetails.contains("deadline") && !tenderDetails["deadline"].isNull()) {
        QString deadlineStr = tenderDetails["deadline"].toString();
        // Пробуем разные форматы
        QDateTime dt = QDateTime::fromString(deadlineStr, "dd.MM.yyyy HH:mm:ss");
        if (!dt.isValid())
            dt = QDateTime::fromString(deadlineStr, "dd.MM.yyyy HH:mm");
        if (!dt.isValid())
            dt = QDateTime::fromString(deadlineStr, "dd.MM.yyyy");
        if (dt.isValid())
            m_dtDeadline->setDateTime(dt);
    }
    
    m_comboStatus->setCurrentIndex(0); // "Новый"
}

void TenderEditDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    
    m_editNoticeNumber = new QLineEdit;
    m_editNoticeNumber->setPlaceholderText("Например: 01234567890123456789");
    form->addRow("№ извещения:", m_editNoticeNumber);
    
    m_editLotNumber = new QLineEdit;
    form->addRow("№ лота:", m_editLotNumber);
    
    m_editPurchaseName = new QLineEdit;
    m_editPurchaseName->setPlaceholderText("Полное название закупки");
    form->addRow("Название закупки*:", m_editPurchaseName);
    
    m_editCustomer = new QLineEdit;
    form->addRow("Заказчик:", m_editCustomer);
    
    m_editEtpUrl = new QLineEdit;
    m_editEtpUrl->setPlaceholderText("https://...");
    form->addRow("Ссылка на ЭТП:", m_editEtpUrl);
    
    auto* nmckLayout = new QHBoxLayout;
    m_spinNmck = new QDoubleSpinBox;
    m_spinNmck->setMaximum(999999999999.99);
    m_spinNmck->setDecimals(2);
    m_spinNmck->setPrefix("₽ ");
    nmckLayout->addWidget(m_spinNmck);
    
    m_comboCurrency = new QComboBox;
    m_comboCurrency->addItems({"RUB", "USD", "EUR"});
    nmckLayout->addWidget(m_comboCurrency);
    form->addRow("НМЦК:", nmckLayout);
    
    m_dtDeadline = new QDateTimeEdit;
    m_dtDeadline->setDisplayFormat("dd.MM.yyyy HH:mm");
    m_dtDeadline->setCalendarPopup(true);
    form->addRow("Дедлайн подачи:", m_dtDeadline);
    
    m_comboStatus = new QComboBox;
    m_comboStatus->addItem("Новый", "NEW");
    m_comboStatus->addItem("В работе", "IN_PROGRESS");
    m_comboStatus->addItem("Заявка подана", "SUBMITTED");
    m_comboStatus->addItem("Выигран", "WON");
    m_comboStatus->addItem("Проигран", "LOST");
    m_comboStatus->addItem("Отменен", "CANCELLED");
    m_comboStatus->addItem("В архиве", "ARCHIVED");
    form->addRow("Статус:", m_comboStatus);
    
    m_editNotes = new QTextEdit;
    m_editNotes->setMaximumHeight(100);
    form->addRow("Заметки:", m_editNotes);
    
    layout->addLayout(form);
    
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttons, &QDialogButtonBox::accepted, this, &TenderEditDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    
    resize(600, 500);
}

void TenderEditDialog::loadTender(int id) {
    m_api->getTender(
        id,
        [this](const dto::Tender& t) {
            m_editNoticeNumber->setText(t.notice_number);
            m_editLotNumber->setText(t.lot_number);
            m_editPurchaseName->setText(t.purchase_name);
            m_editCustomer->setText(t.customer_name);
            m_editEtpUrl->setText(t.etp_url);
            m_spinNmck->setValue(t.nmck);
            
            int idx = m_comboCurrency->findText(t.currency);
            if (idx >= 0) m_comboCurrency->setCurrentIndex(idx);
            
            if (t.application_deadline.isValid())
                m_dtDeadline->setDateTime(t.application_deadline);
            
            idx = m_comboStatus->findData(t.status);
            if (idx >= 0) m_comboStatus->setCurrentIndex(idx);
            
            m_editNotes->setPlainText(t.notes);
        },
        [this](const QString& error) {
            QMessageBox::warning(this, "Ошибка", error);
            reject();
        }
    );
}

void TenderEditDialog::onAccept() {
    if (m_editPurchaseName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите название закупки");
        return;
    }
    
    dto::Tender t;
    t.notice_number = m_editNoticeNumber->text().trimmed();
    t.lot_number = m_editLotNumber->text().trimmed();
    t.purchase_name = m_editPurchaseName->text().trimmed();
    t.customer_name = m_editCustomer->text().trimmed();
    t.etp_url = m_editEtpUrl->text().trimmed();
    t.nmck = m_spinNmck->value();
    t.currency = m_comboCurrency->currentText();
    t.application_deadline = m_dtDeadline->dateTime();
    t.status = m_comboStatus->currentData().toString();
    t.notes = m_editNotes->toPlainText();
    
    auto onSuccess = [this](const dto::Tender&) {
        accept();
    };
    auto onError = [this](const QString& error) {
        QMessageBox::warning(this, "Ошибка", error);
    };
    
    if (m_isNew) {
        m_api->createTender(t, onSuccess, onError);
    } else {
        m_api->updateTender(m_tenderId, t, onSuccess, onError);
    }
}