#include "LinkEmailDialog.h"
#include "TenderEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

LinkEmailDialog::LinkEmailDialog(ApiClient* api, const dto::Email& email, QWidget* parent)
    : QDialog(parent)
    , m_api(api)
    , m_email(email)
{
    setupUi();
    loadTenders();
}

void LinkEmailDialog::setupUi() {
    setWindowTitle("Связать письмо с тендером");
    setMinimumWidth(600);
    
    auto* layout = new QVBoxLayout(this);
    
    // Информация о письме
    auto* emailGroup = new QGroupBox("📧 Письмо");
    auto* emailForm = new QFormLayout(emailGroup);
    
    QString emailInfo = m_email.subject.isEmpty() ? "(без темы)" : m_email.subject;
    m_lblEmailInfo = new QLabel(emailInfo);
    m_lblEmailInfo->setWordWrap(true);
    emailForm->addRow("Тема:", m_lblEmailInfo);
    
    auto* fromLabel = new QLabel(m_email.from_email);
    emailForm->addRow("От:", fromLabel);
    
    if (m_email.tender_details.contains("purchase_name") 
        && !m_email.tender_details["purchase_name"].isNull()) {
        auto* purchaseLabel = new QLabel(m_email.tender_details["purchase_name"].toString());
        purchaseLabel->setWordWrap(true);
        purchaseLabel->setStyleSheet("color: #28a745; font-weight: bold;");
        emailForm->addRow("Извлеченная закупка:", purchaseLabel);
    }
    
    if (m_email.tender_details.contains("nmck") 
        && !m_email.tender_details["nmck"].isNull()) {
        auto* nmckLabel = new QLabel(m_email.tender_details["nmck"].toString());
        nmckLabel->setStyleSheet("color: #28a745; font-weight: bold;");
        emailForm->addRow("Извлеченная НМЦК:", nmckLabel);
    }
    
    layout->addWidget(emailGroup);
    
    // Выбор тендера
    auto* tenderGroup = new QGroupBox("🏆 Выберите тендер");
    auto* tenderLayout = new QVBoxLayout(tenderGroup);
    
    m_comboTenders = new QComboBox;
    m_comboTenders->addItem("Загрузка тендеров...", 0);
    m_comboTenders->setEnabled(false);
    tenderLayout->addWidget(m_comboTenders);
    
    auto* btnLayout = new QHBoxLayout;
    m_btnCreateNew = new QPushButton("➕ Создать новый тендер из письма");
    m_btnCreateNew->setStyleSheet("background-color: #28a745; color: white; padding: 5px;");
    btnLayout->addWidget(m_btnCreateNew);
    btnLayout->addStretch();
    tenderLayout->addLayout(btnLayout);
    
    layout->addWidget(tenderGroup);
    
    // Кнопки
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    m_btnLink = buttonBox->button(QDialogButtonBox::Ok);
    m_btnLink->setText("🔗 Связать");
    m_btnLink->setEnabled(false);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LinkEmailDialog::onLink);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_comboTenders, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LinkEmailDialog::onTenderSelected);
    connect(m_btnCreateNew, &QPushButton::clicked, this, &LinkEmailDialog::onCreateNewTender);
    
    layout->addWidget(buttonBox);
}

void LinkEmailDialog::loadTenders() {
    m_api->getTenders(
        [this](const dto::TendersList& list) {
            m_comboTenders->clear();
            m_tenders = list.items;
            
            if (m_tenders.isEmpty()) {
                m_comboTenders->addItem("Нет тендеров в базе", 0);
                m_btnLink->setEnabled(false);
            } else {
                m_comboTenders->addItem("-- Выберите тендер --", 0);
                for (const auto& t : m_tenders) {
                    QString label = QString("[%1] %2 (%3)")
                        .arg(t.id)
                        .arg(t.purchase_name.left(60))
                        .arg(t.status);
                    m_comboTenders->addItem(label, t.id);
                }
                m_comboTenders->setEnabled(true);
            }
        },
        [this](const QString& error) {
            m_comboTenders->clear();
            m_comboTenders->addItem("Ошибка загрузки: " + error, 0);
            QMessageBox::warning(this, "Ошибка", error);
        },
        1, 100  // Загружаем все тендеры
    );
}

void LinkEmailDialog::onTenderSelected(int index) {
    m_selectedTenderId = m_comboTenders->itemData(index).toInt();
    m_btnLink->setEnabled(m_selectedTenderId > 0);
}

void LinkEmailDialog::onCreateNewTender() {
    // Предзаполняем данные из письма
    TenderEditDialog dialog(m_api, this, m_email.tender_details);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Перезагружаем список тендеров
        loadTenders();
    }
}

void LinkEmailDialog::onLink() {
    if (m_selectedTenderId <= 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите тендер");
        return;
    }
    
    m_btnLink->setEnabled(false);
    m_btnLink->setText("Связывание...");
    
    m_api->linkEmailToTender(
        m_email.id,
        m_selectedTenderId,
        [this]() {
            QMessageBox::information(this, "Успех", 
                QString("Письмо #%1 успешно связано с тендером #%2")
                    .arg(m_email.id)
                    .arg(m_selectedTenderId));
            accept();
        },
        [this](const QString& error) {
            QMessageBox::warning(this, "Ошибка", error);
            m_btnLink->setEnabled(true);
            m_btnLink->setText("🔗 Связать");
        }
    );
}