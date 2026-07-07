#include "dialogs/LinkEmailDialog.h"
#include "dialogs/EmailDetailDialog.h"
#include "EmailsView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QRegularExpression>

// ============ Вспомогательные функции для отображения писем ============

/**
 * Конвертирует Markdown/plain text в HTML для красивого отображения.
 */
static QString convertMarkdownToHtml(const QString& text) {
    QString html = text;
    
    // Экранируем HTML-символы
    html.replace("&", "&amp;");
    html.replace("<", "&lt;");
    html.replace(">", "&gt;");
    
    // Жирный текст: **текст** -> <b>текст</b>
    html.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<b>\\1</b>");
    
    // Курсив: *текст* -> <i>текст</i>
    html.replace(QRegularExpression("(?<!\\*)\\*([^*]+)\\*(?!\\*)"), "<i>\\1</i>");
    
    // Ссылки: [текст](url) -> <a href="url">текст</a>
    html.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"), 
                 "<a href=\"\\2\" style=\"color: #0066cc; text-decoration: underline;\">\\1</a>");
    
    // Горизонтальные линии: --- -> <hr>
    html.replace(QRegularExpression("^---+$", QRegularExpression::MultilineOption), "<hr>");
    
    // Заголовки
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");
    
    // Списки
    html.replace(QRegularExpression("^- (.+)$", QRegularExpression::MultilineOption), "<li>\\1</li>");
    
    // Переносы строк
    html.replace("\n", "<br>");
    
    // Удаляем множественные пустые строки
    html.replace(QRegularExpression("(<br\\s*/?>\\s*){3,}"), "<br><br>");
    
    // Оборачиваем в div с базовыми стилями
    html = QString("<div style='font-family: Arial, sans-serif; font-size: 14px; line-height: 1.5;'>%1</div>").arg(html);
    
    return html;
}

/**
 * Очищает HTML от трекеров, скрытых элементов и невидимых символов.
 * Также исправляет некорректные CSS-свойства, вызывающие предупреждения Qt.
 */
static QString cleanEmailHtml(const QString& html) {
    QString cleaned = html;
    
    // =========================================================================
    // 1. Удаление трекеров и скрытых элементов
    // =========================================================================
    
    // Удаляем tracking pixels (изображения 1px и меньше)
    cleaned.remove(QRegularExpression(
        "<img[^>]*\\s+(?:width|height)\\s*=\\s*[\"']?[01][\"']?[^>]*>",
        QRegularExpression::CaseInsensitiveOption));
    
    // Удаляем скрытые preheader
    cleaned.remove(QRegularExpression(
        "<span[^>]*class\\s*=\\s*[\"'][^\"']*preheader[^\"']*[\"'][^>]*>.*?</span>",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    
    // Удаляем элементы с display:none (инлайновый стиль)
    cleaned.remove(QRegularExpression(
        "<[a-z]+[^>]*style\\s*=\\s*[\"'][^\"']*display\\s*:\\s*none[^\"']*[\"'][^>]*>.*?</[a-z]+>",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    
    // Удаляем элементы с mso-hide (скрытие для Outlook)
    cleaned.remove(QRegularExpression(
        "<[a-z]+[^>]*style\\s*=\\s*[\"'][^\"']*mso-hide\\s*:\\s*all[^\"']*[\"'][^>]*>.*?</[a-z]+>",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
    
    // =========================================================================
    // 2. Очистка невидимых Unicode-символов
    // =========================================================================
    
    cleaned.replace(QRegularExpression("[\\x{200B}\\x{200C}\\x{200D}\\x{2060}\\x{FEFF}]"), " ");
    cleaned.replace(QRegularExpression("\\x{00A0}"), " ");
    cleaned.replace(QRegularExpression("\\x{2028}"), " ");
    cleaned.replace(QRegularExpression("\\x{2029}"), " ");
    cleaned.replace(QRegularExpression("\\x{3000}"), " ");
    cleaned.replace(QRegularExpression("[\\x{2800}-\\x{28FF}]+"), " ");
    
    // =========================================================================
    // 3. Исправление некорректных CSS-свойств
    // =========================================================================
    
    // font-size: 0 → font-size: 12px
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "font-size: 12px;");
    
    // font-size: -Npx (отрицательные)
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*-[0-9.]+\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "font-size: 12px;");
    
    // font-size: 1px (часто для трекеров)
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*1\\s*px\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "font-size: 12px;");
    
    // height: 0 (просто, без lookbehind — даже для <hr> замена безопасна)
    cleaned.replace(QRegularExpression(
        "height\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "");
    
    // height: -N (отрицательные)
    cleaned.replace(QRegularExpression(
        "height\\s*:\\s*-[0-9.]+\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "");
    
    // width: 0
    cleaned.replace(QRegularExpression(
        "width\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "");
    
    // line-height: 0 → line-height: 1.4
    cleaned.replace(QRegularExpression(
        "line-height\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "line-height: 1.4;");
    
    // line-height: -N (отрицательные)
    cleaned.replace(QRegularExpression(
        "line-height\\s*:\\s*-[0-9.]+\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "line-height: 1.4;");
    
    // max-height: 0 → убрать полностью
    cleaned.replace(QRegularExpression(
        "max-height\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "");
    
    // max-width: 0 → убрать полностью
    cleaned.replace(QRegularExpression(
        "max-width\\s*:\\s*0(\\.0+)?\\s*(px|pt|em|rem|%|cm|mm|in|pc|ex|ch)?\\s*;?",
        QRegularExpression::CaseInsensitiveOption),
        "");
    
    // =========================================================================
    // 4. Замена относительных размеров на абсолютные
    // =========================================================================
    
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*xx-small\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 9px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*x-small\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 10px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*small\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 12px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*medium\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 14px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*large\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 18px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*x-large\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 24px");
    cleaned.replace(QRegularExpression(
        "font-size\\s*:\\s*xx-large\\b", QRegularExpression::CaseInsensitiveOption),
        "font-size: 32px");
    
    // =========================================================================
    // 5. Удаление пустых элементов
    // =========================================================================
    
    cleaned.remove(QRegularExpression(
        "<div[^>]*>\\s*</div>",
        QRegularExpression::CaseInsensitiveOption));
    
    cleaned.remove(QRegularExpression(
        "<span[^>]*>\\s*</span>",
        QRegularExpression::CaseInsensitiveOption));
    
    cleaned.remove(QRegularExpression(
        "<p[^>]*>\\s*</p>",
        QRegularExpression::CaseInsensitiveOption));
    
    // =========================================================================
    // 6. Финальная очистка
    // =========================================================================
    
    // Условные комментарии MSO — простая версия без сложного lookaround
    // Удаляем открывающие: <!--[if ...> ... <![endif]-->
    // Для безопасности удалим весь блок между ними отдельным подходом
    while (cleaned.contains("<!--[if")) {
        int start = cleaned.indexOf("<!--[if");
        if (start == -1) break;
        int end = cleaned.indexOf("<![endif]-->", start);
        if (end == -1) break;
        cleaned.remove(start, end - start + 12);  // 12 = длина "<![endif]-->"
    }
    
    // Удаляем множественные <br> (более 3 подряд)
    cleaned.replace(QRegularExpression("(<br\\s*/?>\\s*){3,}", QRegularExpression::CaseInsensitiveOption), "<br><br>");
    
    // Удаляем множественные пробелы
    cleaned.replace(QRegularExpression(" {3,}"), "  ");
    
    return cleaned;
}

EmailsView::EmailsView(ApiClient* api, QWidget* parent)
    : QWidget(parent)
    , m_api(api)
    , m_currentPage(1)
    , m_perPage(50)
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
	m_table->setSortingEnabled(true);
    m_table->sortByColumn(EmailsModel::ColDate, Qt::DescendingOrder);
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

    // ====== НОВАЯ ПАНЕЛЬ ПАГИНАЦИИ ======
    auto* paginationBar = new QWidget;
    auto* pagLayout = new QHBoxLayout(paginationBar);
    pagLayout->setContentsMargins(5, 2, 5, 2);

    pagLayout->addWidget(new QLabel("Показывать:"));

    m_comboPerPage = new QComboBox;
    m_comboPerPage->addItem("25", 25);
    m_comboPerPage->addItem("50", 50);
    m_comboPerPage->addItem("100", 100);
    m_comboPerPage->addItem("500", 500);
    m_comboPerPage->addItem("1000", 1000);
    m_comboPerPage->addItem("5000", 5000);
    m_comboPerPage->addItem("Все письма", 15000);
    m_comboPerPage->setCurrentIndex(1); // по умолчанию 50
    m_comboPerPage->setMaximumWidth(150);
    pagLayout->addWidget(m_comboPerPage);

    pagLayout->addSpacing(10);

    m_btnPrev = new QPushButton("← Назад");
    m_btnPrev->setEnabled(false);
    m_btnPrev->setMaximumWidth(80);
    pagLayout->addWidget(m_btnPrev);

    m_lblPageInfo = new QLabel("Страница 1");
    m_lblPageInfo->setAlignment(Qt::AlignCenter);
    m_lblPageInfo->setStyleSheet("color: #666;");
    pagLayout->addWidget(m_lblPageInfo, 1);

    m_btnNext = new QPushButton("Вперёд →");
    m_btnNext->setEnabled(false);
    m_btnNext->setMaximumWidth(80);
    pagLayout->addWidget(m_btnNext);

    leftLayout->addWidget(paginationBar);

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
	m_txtBody = new EmailTextBrowser;
	m_txtBody->setStyleSheet(
		"EmailTextBrowser { "
		"  background-color: #ffffff; "
		"  border: 1px solid #ddd; "
		"  border-radius: 4px; "
		"  padding: 8px; "
		"}"
	);
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
    
    // Пагинация
    connect(m_btnPrev, &QPushButton::clicked, this, &EmailsView::onPrevPage);
    connect(m_btnNext, &QPushButton::clicked, this, &EmailsView::onNextPage);
    connect(m_comboPerPage, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EmailsView::onPerPageChanged);
    
    connect(m_model, &EmailsModel::totalChanged, this, [this](int total) {
        m_lblTotal->setText(QString("Всего: %1").arg(total));
        updatePaginationUI(total);
    });
    
    connect(m_model, &EmailsModel::errorOccurred, this, [this](const QString& error) {
        QMessageBox::warning(this, "Ошибка", error);
    });
}

void EmailsView::refresh() {
    m_currentPage = 1;
    loadCurrentPage();
}

void EmailsView::loadCurrentPage() {
    QString category = m_categoryFilter->currentData().toString();
    QString search = m_searchEdit->text();
    m_model->load(m_currentPage, category, search, m_perPage);
}

void EmailsView::updatePaginationUI(int total) {
    int totalPages = (total + m_perPage - 1) / m_perPage;
    if (totalPages == 0) totalPages = 1;
    
    m_lblPageInfo->setText(QString("Страница %1 из %2").arg(m_currentPage).arg(totalPages));
    
    m_btnPrev->setEnabled(m_currentPage > 1);
    m_btnNext->setEnabled(m_currentPage < totalPages);
    
    // Обновляем текст "Все письма (N)"
    int lastIndex = m_comboPerPage->count() - 1;
    m_comboPerPage->setItemText(lastIndex, QString("Все письма (%1)").arg(total));
}

void EmailsView::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        loadCurrentPage();
    }
}

void EmailsView::onNextPage() {
    m_currentPage++;
    loadCurrentPage();
}

void EmailsView::onPerPageChanged() {
    int newPerPage = m_comboPerPage->currentData().toInt();
    
    // Предупреждение для больших объёмов
    if (newPerPage >= 5000) {
        auto result = QMessageBox::question(
            this,
            "Загрузка большого количества писем",
            QString("Вы собираетесь загрузить до %1 писем.\n\n"
                    "Это может занять 10-30 секунд и потребить больше памяти.\n\n"
                    "Продолжить?").arg(newPerPage),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (result != QMessageBox::Yes) {
            m_comboPerPage->setCurrentIndex(1); // Возврат к 50
            return;
        }
    }
    
    m_perPage = newPerPage;
    m_currentPage = 1;
    loadCurrentPage();
}

void EmailsView::onRowClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    const auto& email = m_model->emailAt(index.row());
    updatePreview(email);
    
    m_btnLink->setEnabled(email.category == "TENDER");
}

void EmailsView::onRowDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    const auto& email = m_model->emailAt(index.row());
    
    // Открываем детальный диалог
    auto* dialog = new EmailDetailDialog(m_api, email.id, this);
    dialog->exec();
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
			if (size > 1024 * 1024) {
				QString sizeStr = QString::number(size / 1024.0 / 1024.0, 'f', 1);
				names << QString("%1 (%2 МБ)").arg(name, sizeStr);
			} else if (size > 1024) {
				QString sizeStr = QString::number(size / 1024.0, 'f', 1);
				names << QString("%1 (%2 КБ)").arg(name, sizeStr);
			} else {
				names << QString("%1 (%2 байт)").arg(name).arg(size);
			}
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
    // Отображение тела письма: HTML с форматированием или Markdown
	if (!email.body_html.isEmpty()) {
		QString cleanedHtml = cleanEmailHtml(email.body_html);
		m_txtBody->setEmailHtml(cleanedHtml);
	} else if (!email.body_text.isEmpty()) {
		m_txtBody->setEmailHtml(convertMarkdownToHtml(email.body_text));
	} else {
		m_txtBody->setPlainText("(пустое письмо)");
	}
}

void EmailsView::onSearch() {
    m_currentPage = 1;
    loadCurrentPage();
}

void EmailsView::onCategoryFilterChanged() {
    m_currentPage = 1;
    loadCurrentPage();
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