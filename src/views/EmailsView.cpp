#include "dialogs/LinkEmailDialog.h"
#include "EmailsView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>

EmailsView::EmailsView(ApiClient* api, QWidget* parent)
    : QWidget(parent)
    , m_api(api)
{
    setupUi();
    setupConnections();
}

void EmailsView::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    // ====== Левая панель: список писем ======
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Панель фильтров
    auto* toolbar = new QHBoxLayout;

    m_categoryFilter = new QComboBox;
    m_categoryFilter->addItem("Все категории", "");
    m_categoryFilter->addItem("🏆 TENDER", "TENDER");
    m_categoryFilter->addItem("🗑 SPAM", "SPAM");
    m_categoryFilter->addItem("📋 GENERAL", "GENERAL");
    m_categoryFilter->addItem("⬜ EMPTY", "EMPTY");
    toolbar->addWidget(m_categoryFilter);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Поиск...");
    m_searchEdit->setMinimumWidth(200);
    toolbar->addWidget(m_searchEdit);

    m_btnRefresh = new QPushButton("🔄");
    m_btnRefresh->setMaximumWidth(40);
    toolbar->addWidget(m_btnRefresh);

    leftLayout->addLayout(toolbar);

    // Таблица
    m_model = new EmailsModel(m_api, this);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColSubject, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColId, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColDate, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColCategory, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColFrom, QHeaderView::Interactive);
    m_table->setColumnWidth(EmailsModel::ColFrom, 200);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColSummary, QHeaderView::Interactive);
    m_table->setColumnWidth(EmailsModel::ColSummary, 300);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColAttachments, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(EmailsModel::ColLinked, QHeaderView::ResizeToContents);

    leftLayout->addWidget(m_table, 1);

    // Нижняя панель
    auto* bottomBar = new QHBoxLayout;
    m_lblTotal = new QLabel("Всего: 0");
    bottomBar->addWidget(m_lblTotal);
    bottomBar->addStretch();

    m_btnLink = new QPushButton("🔗 Связать с тендером");
    m_btnLink->setEnabled(false);
    bottomBar->addWidget(m_btnLink);

    leftLayout->addLayout(bottomBar);

    // ====== Правая панель: предпросмотр ======
    auto* rightScroll = new QScrollArea;
    rightScroll->setWidgetResizable(true);
    rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);

    // Заголовок
    m_lblSubject = new QLabel("Выберите письмо");
    m_lblSubject->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_lblSubject->setWordWrap(true);
    rightLayout->addWidget(m_lblSubject);

    // Метаданные
    auto* metaGroup = new QGroupBox("📋 Информация");
    auto* metaForm = new QFormLayout(metaGroup);

    m_lblFrom = new QLabel("-");
    m_lblFrom->setTextInteractionFlags(Qt::TextSelectableByMouse);
    metaForm->addRow("От:", m_lblFrom);

    m_lblDate = new QLabel("-");
    metaForm->addRow("Дата:", m_lblDate);

    m_lblCategory = new QLabel("-");
    metaForm->addRow("Категория:", m_lblCategory);

    m_lblAttachments = new QLabel("-");
    m_lblAttachments->setWordWrap(true);
    metaForm->addRow("Вложения:", m_lblAttachments);

    rightLayout->addWidget(metaGroup);

    // ====== БЛОК ТЕНДЕРНЫХ ДАННЫХ (отдельная группа) ======
    auto* tenderGroup = new QGroupBox("🏆 Тендерные данные (извлечено LLM)");
    tenderGroup->setStyleSheet(
        "QGroupBox { "
        "  font-weight: bold; "
        "  border: 2px solid #28a745; "
        "  border-radius: 5px; "
        "  margin-top: 10px; "
        "  padding-top: 10px; "
        "  background-color: #f0fff0; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "  color: #28a745; "
        "}"
        );
    auto* tenderLayout = new QVBoxLayout(tenderGroup);

    m_lblTenderDetails = new QLabel("Нет тендерных данных");
    m_lblTenderDetails->setWordWrap(true);
    m_lblTenderDetails->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lblTenderDetails->setStyleSheet("color: #666; font-weight: normal;");
    tenderLayout->addWidget(m_lblTenderDetails);

    rightLayout->addWidget(tenderGroup);

    // Резюме
    auto* summaryGroup = new QGroupBox("📝 Резюме LLM");
    auto* summaryLayout = new QVBoxLayout(summaryGroup);
    m_txtSummary = new QTextEdit;
    m_txtSummary->setReadOnly(true);
    m_txtSummary->setMaximumHeight(80);
    summaryLayout->addWidget(m_txtSummary);
    rightLayout->addWidget(summaryGroup);

    // Тело письма
    auto* bodyGroup = new QGroupBox("📄 Текст письма");
    auto* bodyLayout = new QVBoxLayout(bodyGroup);
    m_txtBody = new QTextEdit;
    m_txtBody->setReadOnly(true);
    bodyLayout->addWidget(m_txtBody);
    rightLayout->addWidget(bodyGroup, 1);

    // Собираем в scroll area
    rightScroll->setWidget(rightPanel);

    // Собираем splitter
    m_splitter->addWidget(leftPanel);
    m_splitter->addWidget(rightScroll);
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(m_splitter);
}

void EmailsView::setupConnections() {
    connect(m_table, &QTableView::clicked, this, &EmailsView::onRowClicked);
    connect(m_table, &QTableView::doubleClicked, this, &EmailsView::onRowDoubleClicked);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &EmailsView::onSearch);
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EmailsView::onCategoryFilterChanged);
    connect(m_btnRefresh, &QPushButton::clicked, this, &EmailsView::refresh);
    connect(m_btnLink, &QPushButton::clicked, this, &EmailsView::onLinkToTender);
    
    connect(m_model, &EmailsModel::totalChanged, this, [this](int total) {
        m_lblTotal->setText(QString("Всего: %1").arg(total));
    });
    
    connect(m_model, &EmailsModel::errorOccurred, this, [this](const QString& error) {
        QMessageBox::warning(this, "Ошибка", error);
    });
}

void EmailsView::refresh() {
    QString category = m_categoryFilter->currentData().toString();
    QString search = m_searchEdit->text();
    m_model->load(1, category, search);
}

void EmailsView::onRowClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    const auto& email = m_model->emailAt(index.row());
    updatePreview(email);
    
    m_btnLink->setEnabled(email.category == "TENDER");
}

void EmailsView::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    // TODO: открыть детальный диалог
    const auto& email = m_model->emailAt(index.row());
    QMessageBox::information(this, "Письмо",
        QString("ID: %1\nТема: %2\n\n(Детальный просмотр будет добавлен)").arg(email.id).arg(email.subject));
}

void EmailsView::updatePreview(const dto::Email& email) {
    m_lblSubject->setText(email.subject.isEmpty() ? "(без темы)" : email.subject);

    QString from = email.from_email;
    if (!email.from_name.isEmpty())
        from = QString("%1 <%2>").arg(email.from_name, email.from_email);
    m_lblFrom->setText(from);

    m_lblDate->setText(email.email_date.isValid()
                           ? email.email_date.toString("dd.MM.yyyy HH:mm")
                           : "н/д");

    m_lblCategory->setText(email.category);

    // Вложения
    if (email.attachments_info.isEmpty()) {
        m_lblAttachments->setText("нет");
    } else {
        QStringList names;
        for (const auto& att : email.attachments_info) {
            auto obj = att.toObject();
            QString name = obj["filename"].toString();
            int size = obj["size_bytes"].toInt();
            if (size > 1024 * 1024)
                names << QString("%1 (%.1f МБ)").arg(name).arg(size / 1024.0 / 1024.0);
            else if (size > 1024)
                names << QString("%1 (%.1f КБ)").arg(name).arg(size / 1024.0);
            else
                names << name;
        }
        m_lblAttachments->setText(names.join("\n"));
    }

    // Тендерные данные — красивое отображение
    if (!email.tender_details.isEmpty()) {
        const auto& td = email.tender_details;
        QStringList details;

        if (td.contains("notice_number") && !td["notice_number"].isNull()
            && !td["notice_number"].toString().isEmpty()) {
            details << "🔢 <b>№ извещения:</b> " + td["notice_number"].toString();
        }
        if (td.contains("purchase_name") && !td["purchase_name"].isNull()
            && !td["purchase_name"].toString().isEmpty()) {
            details << "🏗 <b>Закупка:</b> " + td["purchase_name"].toString();
        }
        if (td.contains("nmck") && !td["nmck"].isNull()
            && !td["nmck"].toString().isEmpty()) {
            details << "💰 <b>НМЦК:</b> " + td["nmck"].toString();
        }
        if (td.contains("deadline") && !td["deadline"].isNull()
            && !td["deadline"].toString().isEmpty()) {
            details << "⏰ <b>Срок подачи:</b> " + td["deadline"].toString();
        }

        if (!details.isEmpty()) {
            m_lblTenderDetails->setTextFormat(Qt::RichText);
            m_lblTenderDetails->setText(details.join("<br>"));
            m_lblTenderDetails->setStyleSheet("color: #000; font-weight: normal;");
        } else {
            m_lblTenderDetails->setText("Категория TENDER, но детали не извлечены");
            m_lblTenderDetails->setStyleSheet("color: #999; font-weight: normal; font-style: italic;");
        }
    } else {
        m_lblTenderDetails->setText("Нет тендерных данных");
        m_lblTenderDetails->setStyleSheet("color: #999; font-weight: normal; font-style: italic;");
    }

    m_txtSummary->setPlainText(email.summary);
    m_txtBody->setPlainText(email.body_text);
}

void EmailsView::onSearch() {
    refresh();
}

void EmailsView::onCategoryFilterChanged() {
    refresh();
}

void EmailsView::onLinkToTender() {
    auto index = m_table->currentIndex();
    if (!index.isValid()) return;
    
    const auto& email = m_model->emailAt(index.row());
    
    LinkEmailDialog dialog(m_api, email, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Обновляем список писем (чтобы показать связь)
        refresh();
    }
}
