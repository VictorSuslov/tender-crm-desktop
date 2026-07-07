#include "SettingsView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QDateTime>
#include <QJsonArray>

SettingsView::SettingsView(ApiClient* api, QWidget* parent)
    : QWidget(parent)
    , m_api(api)
    , m_refreshTimer(new QTimer(this))
{
    setupUi();
    setupConnections();
	
	// Инициализируем режим обработки
    onProcessModeChanged();

    // Загружаем статистику при старте
    loadStatistics();
}

void SettingsView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // ===== Группа: Подключение к серверу =====
    auto* connectionGroup = new QGroupBox("🔌 Подключение к серверу");
    auto* connectionLayout = new QVBoxLayout(connectionGroup);

    auto* urlLayout = new QHBoxLayout;
    urlLayout->addWidget(new QLabel("URL API:"));

    m_editApiUrl = new QLineEdit;
    m_editApiUrl->setText("http://localhost:8000");
    m_editApiUrl->setPlaceholderText("http://localhost:8000");
    urlLayout->addWidget(m_editApiUrl, 1);

    m_btnTestConnection = new QPushButton("🔍 Проверить");
    m_btnTestConnection->setMaximumWidth(120);
    urlLayout->addWidget(m_btnTestConnection);

    connectionLayout->addLayout(urlLayout);

    m_lblConnectionStatus = new QLabel("Статус: не проверено");
    m_lblConnectionStatus->setStyleSheet("color: #666; font-style: italic;");
    connectionLayout->addWidget(m_lblConnectionStatus);

    mainLayout->addWidget(connectionGroup);
		
	// ===== Группа: Статус почтового ящика =====
	m_imapStatusGroup = new QGroupBox("📧 Статус почтового ящика");
	auto* imapLayout = new QVBoxLayout(m_imapStatusGroup);

	auto* imapTopLayout = new QHBoxLayout;

	m_lblImapStatus = new QLabel("Статус: не проверено");
	m_lblImapStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #666;");
	imapTopLayout->addWidget(m_lblImapStatus, 1);

	m_btnCheckImap = new QPushButton("🔍 Проверить");
	m_btnCheckImap->setMaximumWidth(120);
	m_btnCheckImap->setStyleSheet(
		"QPushButton { background-color: #17a2b8; color: white; padding: 8px; font-weight: bold; }"
		"QPushButton:hover { background-color: #138496; }"
		"QPushButton:disabled { background-color: #6c757d; }"
	);
	imapTopLayout->addWidget(m_btnCheckImap);

	imapLayout->addLayout(imapTopLayout);

	m_lblImapDetails = new QLabel("");
	m_lblImapDetails->setWordWrap(true);
	m_lblImapDetails->setStyleSheet("color: #666; margin-top: 5px;");
	imapLayout->addWidget(m_lblImapDetails);

	// Контейнер для шагов диагностики
	m_imapStepsWidget = new QWidget;
	m_imapStepsLayout = new QVBoxLayout(m_imapStepsWidget);
	m_imapStepsLayout->setContentsMargins(10, 5, 10, 5);
	m_imapStepsLayout->setSpacing(3);
	m_imapStepsWidget->setVisible(false);
	imapLayout->addWidget(m_imapStepsWidget);

	mainLayout->addWidget(m_imapStatusGroup);

    // ===== Группа: Обработка почты =====
    auto* processingGroup = new QGroupBox("📧 Обработка почты");
    auto* processingLayout = new QVBoxLayout(processingGroup);

    // Режим 1: Последние N писем
    auto* mode1Layout = new QHBoxLayout;
    m_radioLastN = new QRadioButton("Обработать последние");
    m_radioLastN->setChecked(true);
    mode1Layout->addWidget(m_radioLastN);

    m_spinLimit = new QSpinBox;
    m_spinLimit->setRange(10, 500);
    m_spinLimit->setValue(50);
    m_spinLimit->setSuffix(" писем");
    m_spinLimit->setMaximumWidth(120);
    mode1Layout->addWidget(m_spinLimit);

    mode1Layout->addStretch();
    processingLayout->addLayout(mode1Layout);

    // Режим 2: С определённой даты
    auto* mode2Layout = new QHBoxLayout;
    m_radioSinceDate = new QRadioButton("Обработать с даты");
    mode2Layout->addWidget(m_radioSinceDate);

    m_comboPeriod = new QComboBox;
    m_comboPeriod->addItem("Последний день", "1");
    m_comboPeriod->addItem("Последняя неделя", "7");
    m_comboPeriod->addItem("Последний месяц", "30");
    m_comboPeriod->addItem("Последние 3 месяца", "90");
    m_comboPeriod->addItem("Последний год", "365");
    m_comboPeriod->addItem("Выбрать дату...", "custom");
    m_comboPeriod->setMaximumWidth(180);
    mode2Layout->addWidget(m_comboPeriod);

    m_dateEdit = new QDateEdit;
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDate(QDate::currentDate().addDays(-30));
    m_dateEdit->setDisplayFormat("dd.MM.yyyy");
    m_dateEdit->setMaximumWidth(120);
    m_dateEdit->setEnabled(false);
    m_dateEdit->setToolTip("Выберите дату, с которой нужно обработать письма");
    mode2Layout->addWidget(m_dateEdit);

    mode2Layout->addStretch();
    processingLayout->addLayout(mode2Layout);

    // Чекбокс автообновления статистики
    auto* autoRefreshLayout = new QHBoxLayout;
    m_chkAutoRefresh = new QCheckBox("Автообновление статистики каждые 30 сек");
    m_chkAutoRefresh->setChecked(true);
    autoRefreshLayout->addWidget(m_chkAutoRefresh);
    autoRefreshLayout->addStretch();
    processingLayout->addLayout(autoRefreshLayout);

    // Кнопка обработки
    m_btnProcessNow = new QPushButton("🔄 Обработать почту сейчас");
    m_btnProcessNow->setStyleSheet(
        "QPushButton { background-color: #007bff; color: white; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:disabled { background-color: #6c757d; }"
        );
    processingLayout->addWidget(m_btnProcessNow);

    m_lblProcessStatus = new QLabel("");
    m_lblProcessStatus->setStyleSheet("color: #666;");
    processingLayout->addWidget(m_lblProcessStatus);

    mainLayout->addWidget(processingGroup);

    // ===== Группа: Статистика системы =====
    auto* statsGroup = new QGroupBox("📊 Статистика системы");
    auto* statsLayout = new QGridLayout(statsGroup);

    // Письма
    auto* emailsLabel = new QLabel("📧 Писем в базе:");
    emailsLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(emailsLabel, 0, 0);
    m_lblTotalEmails = new QLabel("—");
    statsLayout->addWidget(m_lblTotalEmails, 0, 1);

    auto* tenderEmailsLabel = new QLabel("🏆 Тендерных:");
    statsLayout->addWidget(tenderEmailsLabel, 1, 0);
    m_lblTenderEmails = new QLabel("—");
    m_lblTenderEmails->setStyleSheet("color: #28a745; font-weight: bold;");
    statsLayout->addWidget(m_lblTenderEmails, 1, 1);

    auto* spamEmailsLabel = new QLabel("🗑 Спам:");
    statsLayout->addWidget(spamEmailsLabel, 2, 0);
    m_lblSpamEmails = new QLabel("—");
    m_lblSpamEmails->setStyleSheet("color: #dc3545;");
    statsLayout->addWidget(m_lblSpamEmails, 2, 1);

    // Тендеры
    auto* tendersLabel = new QLabel("📋 Тендеров:");
    tendersLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(tendersLabel, 0, 2);
    m_lblTotalTenders = new QLabel("—");
    statsLayout->addWidget(m_lblTotalTenders, 0, 3);

    // Документы
    auto* docsLabel = new QLabel("📄 Документов:");
    docsLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(docsLabel, 1, 2);
    m_lblTotalDocuments = new QLabel("—");
    statsLayout->addWidget(m_lblTotalDocuments, 1, 3);

    auto* indexedLabel = new QLabel("✅ Индексировано:");
    statsLayout->addWidget(indexedLabel, 2, 2);
    m_lblIndexedDocuments = new QLabel("—");
    m_lblIndexedDocuments->setStyleSheet("color: #28a745;");
    statsLayout->addWidget(m_lblIndexedDocuments, 2, 3);

    // RAG
    auto* chunksLabel = new QLabel("🧩 Чанков:");
    chunksLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(chunksLabel, 3, 0);
    m_lblTotalChunks = new QLabel("—");
    statsLayout->addWidget(m_lblTotalChunks, 3, 1);

    auto* ragLabel = new QLabel("🤖 RAG-запросов:");
    statsLayout->addWidget(ragLabel, 3, 2);
    m_lblRagQueries = new QLabel("—");
    statsLayout->addWidget(m_lblRagQueries, 3, 3);

    mainLayout->addWidget(statsGroup);

    // ===== Группа: О программе =====
    auto* aboutGroup = new QGroupBox("ℹ О программе");
    auto* aboutLayout = new QFormLayout(aboutGroup);

    aboutLayout->addRow("Версия:", new QLabel("1.0.0"));
    aboutLayout->addRow("LLM модель:", new QLabel("Qwen 2.5 7B (Ollama)"));
    aboutLayout->addRow("Модель эмбеддингов:", new QLabel("intfloat/multilingual-e5-large"));
    aboutLayout->addRow("База данных:", new QLabel("PostgreSQL 16 + pgvector"));
    aboutLayout->addRow("Фреймворк:", new QLabel("Qt 6.10 / C++17"));

    auto* lblAuthor = new QLabel("Автор: Виктор (Sevisu)");
    lblAuthor->setStyleSheet("color: #666;");
    aboutLayout->addRow("", lblAuthor);

    mainLayout->addWidget(aboutGroup);

    mainLayout->addStretch();
}

void SettingsView::setupConnections() {
    connect(m_btnTestConnection, &QPushButton::clicked, this, &SettingsView::onTestConnection);
    connect(m_btnProcessNow, &QPushButton::clicked, this, &SettingsView::onProcessNow);
    connect(m_chkAutoRefresh, &QCheckBox::toggled, this, &SettingsView::onAutoRefreshToggled);
	connect(m_btnCheckImap, &QPushButton::clicked, this, &SettingsView::onCheckImap);
	
	connect(m_radioLastN, &QRadioButton::toggled, this, &SettingsView::onProcessModeChanged);
	connect(m_radioSinceDate, &QRadioButton::toggled, this, &SettingsView::onProcessModeChanged);
	connect(m_comboPeriod, QOverload<int>::of(&QComboBox::currentIndexChanged), 
        this, &SettingsView::onProcessModeChanged);

    // Таймер автообновления статистики (каждые 30 секунд)
    connect(m_refreshTimer, &QTimer::timeout, this, &SettingsView::loadStatistics);
    m_refreshTimer->start(30000);
}

void SettingsView::onTestConnection() {
    m_btnTestConnection->setEnabled(false);
    m_btnTestConnection->setText("Проверка...");
    m_lblConnectionStatus->setText("Статус: проверка соединения...");
    m_lblConnectionStatus->setStyleSheet("color: #666; font-style: italic;");

    m_api->pingServer(
        [this]() {
            updateConnectionStatus(true, "Соединение установлено ✓");
            m_btnTestConnection->setEnabled(true);
            m_btnTestConnection->setText("🔍 Проверить");
        },
        [this](const QString& error) {
            updateConnectionStatus(false, "Ошибка: " + error);
            m_btnTestConnection->setEnabled(true);
            m_btnTestConnection->setText("🔍 Проверить");
        }
        );
}

void SettingsView::updateConnectionStatus(bool connected, const QString& message) {
    if (connected) {
        m_lblConnectionStatus->setText("Статус: " + message);
        m_lblConnectionStatus->setStyleSheet("color: #28a745; font-weight: bold;");
    } else {
        m_lblConnectionStatus->setText("Статус: " + message);
        m_lblConnectionStatus->setStyleSheet("color: #dc3545; font-weight: bold;");
    }
}

void SettingsView::onProcessNow() {
    m_btnProcessNow->setEnabled(false);
    m_btnProcessNow->setText("⏳ Обработка...");
    
    // Определяем параметры
    int limit = 50;
    QString sinceDate = "";
    
    if (m_radioLastN->isChecked()) {
        limit = m_spinLimit->value();
        m_lblProcessStatus->setText(
            QString("Обработка последних %1 писем. Это может занять 2-5 минут...")
                .arg(limit)
        );
    } else {
        QDate date = m_dateEdit->date();
        sinceDate = date.toString("yyyy-MM-dd");
        m_lblProcessStatus->setText(
            QString("Обработка писем с %1. Это может занять 2-5 минут...")
                .arg(date.toString("dd.MM.yyyy"))
        );
    }
    
    m_lblProcessStatus->setStyleSheet("color: #007bff;");
    
    m_api->processEmails(
        limit,
        sinceDate,
        [this]() {
            m_btnProcessNow->setEnabled(true);
            m_btnProcessNow->setText("🔄 Обработать почту сейчас");
            m_lblProcessStatus->setText("Обработка завершена. Обновите список писем (F5).");
            m_lblProcessStatus->setStyleSheet("color: #28a745; font-weight: bold;");
            
            // Обновляем статистику
            loadStatistics();
            
            QMessageBox::information(this, "Успех", 
                "Обработка писем завершена.\n\n"
                "Новые письма добавлены в базу данных.");
        },
        [this](const QString& error) {
            m_btnProcessNow->setEnabled(true);
            m_btnProcessNow->setText("🔄 Обработать почту сейчас");
            m_lblProcessStatus->setText("Ошибка: " + error);
            m_lblProcessStatus->setStyleSheet("color: #dc3545;");
            QMessageBox::warning(this, "Ошибка", error);
        }
    );
}

void SettingsView::onAutoRefreshToggled(bool checked) {
    if (checked) {
        m_refreshTimer->start(30000);
    } else {
        m_refreshTimer->stop();
    }
}

void SettingsView::loadStatistics() {
    // Получаем статистику документов
    m_api->getStatistics(
        [this](const QJsonObject& stats) {
            // Документы
            m_lblTotalDocuments->setText(QString::number(stats["total_documents"].toInt()));
            m_lblIndexedDocuments->setText(QString::number(stats["indexed_documents"].toInt()));
            m_lblTotalChunks->setText(QString::number(stats["total_chunks"].toInt()));

            // Письма — получаем из статистики по категориям
            QJsonObject byType = stats["by_type"].toObject();
            int totalEmails = 0;
            for (auto it = byType.begin(); it != byType.end(); ++it) {
                totalEmails += it.value().toInt();
            }

            // Получаем статистику писем через отдельный запрос
            m_api->getEmails(
                [this](const dto::EmailsList& list) {
                    m_lblTotalEmails->setText(QString::number(list.total));

                    // Считаем по категориям
                    int tender = 0, spam = 0;
                    for (const auto& email : list.items) {
                        if (email.category == "TENDER") tender++;
                        else if (email.category == "SPAM") spam++;
                    }

                    // Если загружена первая страница, показываем примерные значения
                    m_lblTenderEmails->setText(QString("~%1").arg(tender));
                    m_lblSpamEmails->setText(QString("~%1").arg(spam));
                },
                [](const QString&) {
                    // Тихо игнорируем ошибку
                },
                1, 50, "", ""  // Первая страница, 50 писем
                );

            // Тендеры
            m_api->getTenders(
                [this](const dto::TendersList& list) {
                    m_lblTotalTenders->setText(QString::number(list.total));
                },
                [](const QString&) {},
                1, 1
                );

            // RAG запросы (через прямой HTTP)
            // Пока оставим заглушку
            m_lblRagQueries->setText("—");
        },
        [this](const QString& error) {
            m_lblTotalDocuments->setText("Ошибка");
            m_lblTotalDocuments->setStyleSheet("color: #dc3545;");
        }
        );
}

void SettingsView::refresh() {
    loadStatistics();
	
	// Автоматическая проверка IMAP при первом открытии
    static bool firstLoad = true;
    if (firstLoad) {
        firstLoad = false;
        onCheckImap();
    }
}

void SettingsView::onRefreshStatistics() {
    loadStatistics();
}

void SettingsView::onCheckImap() {
    // Защита от null-указателей
    if (!m_btnCheckImap || !m_lblImapStatus || !m_lblImapDetails || !m_imapStepsWidget) {
        qDebug() << "Error: UI elements not initialized";
        return;
    }
    
    // Блокируем кнопку
    m_btnCheckImap->setEnabled(false);
    m_btnCheckImap->setText("⏳ Проверка...");
    
    m_lblImapStatus->setText("Статус: выполняется проверка...");
    m_lblImapStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #007bff;");
    m_lblImapDetails->setText("Подключение к почтовому серверу...");
    
    // Очищаем предыдущие шаги
    if (m_imapStepsLayout) {
        QLayoutItem* item;
        while ((item = m_imapStepsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }
    m_imapStepsWidget->setVisible(false);
    
    m_api->checkImapConnection(
        [this](const QJsonObject& result) {
            // Защита от null
            if (!m_btnCheckImap || !m_lblImapStatus) return;
            
            m_btnCheckImap->setEnabled(true);
            m_btnCheckImap->setText("🔍 Проверить");
            
            bool success = result["success"].toBool();
            QString summary = result["summary"].toString();
            QString server = result["server"].toString();
            QString login = result["login"].toString();
            int inboxCount = result["inbox_count"].toInt();
            
            // Обновляем основной статус
            if (success) {
                m_lblImapStatus->setText("✓ Подключение работает");
                m_lblImapStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #28a745;");
            } else {
                m_lblImapStatus->setText("✗ Ошибка подключения");
                m_lblImapStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #dc3545;");
            }
            
            // Детали
            QString details = QString("Сервер: %1 | Пользователь: %2 | Писем в INBOX: %3")
                .arg(server)
                .arg(login)
                .arg(inboxCount);
            m_lblImapDetails->setText(details);
            
            // Показываем шаги диагностики
            if (m_imapStepsLayout && m_imapStepsWidget) {
                QJsonArray steps = result["steps"].toArray();
                for (const auto& stepValue : steps) {
                    QJsonObject step = stepValue.toObject();
                    
                    QString name = step["name"].toString();
                    QString status = step["status"].toString();
                    QString message = step["message"].toString();
                    QString stepDetails = step["details"].toString();
                    
                    QString icon = (status == "ok") ? "✅" : "❌";
                    QString color = (status == "ok") ? "#28a745" : "#dc3545";
                    
                    auto* stepLabel = new QLabel(
                        QString("<span style='color:%1'>%2 %3:</span> %4 <span style='color:#666; font-size:11px'>(%5)</span>")
                            .arg(color).arg(icon).arg(name).arg(message).arg(stepDetails)
                    );
                    stepLabel->setTextFormat(Qt::RichText);
                    stepLabel->setWordWrap(true);
                    
                    m_imapStepsLayout->addWidget(stepLabel);
                }
                
                m_imapStepsWidget->setVisible(true);
            }
            
            // Итоговое сообщение
            if (!success) {
                QMessageBox::warning(
                    this,
                    "Ошибка подключения к почте",
                    summary + "\n\n"
                    "Проверьте:\n"
                    "• Настройки Яндекса: включён ли доступ по IMAP\n"
                    "• Пароль: если включена 2FA, используйте пароль приложения\n"
                    "• Firewall и антивирус"
                );
            }
        },
        [this](const QString& error) {
            if (!m_btnCheckImap || !m_lblImapStatus) return;
            
            m_btnCheckImap->setEnabled(true);
            m_btnCheckImap->setText("🔍 Проверить");
            
            m_lblImapStatus->setText("✗ Ошибка проверки");
            m_lblImapStatus->setStyleSheet("font-size: 14px; font-weight: bold; color: #dc3545;");
            m_lblImapDetails->setText("Не удалось связаться с сервером Tender CRM");
            
            QMessageBox::warning(
                this,
                "Ошибка",
                "Не удалось выполнить проверку:\n" + error + "\n\n"
                "Убедитесь, что backend запущен."
            );
        }
    );
}

void SettingsView::onProcessModeChanged() {
    // Защита от null
    if (!m_radioLastN || !m_radioSinceDate || !m_spinLimit || !m_comboPeriod || !m_dateEdit) {
        qDebug() << "Error: Process mode elements not initialized";
        return;
    }
    
    bool isLastN = m_radioLastN->isChecked();
    
    // Включаем/выключаем элементы в зависимости от режима
    m_spinLimit->setEnabled(isLastN);
    m_comboPeriod->setEnabled(!isLastN);
    
    // Показываем/скрываем и ВКЛЮЧАЕМ/ВЫКЛЮЧАЕМ dateEdit
    bool showDate = !isLastN && m_comboPeriod->currentData().toString() == "custom";
    m_dateEdit->setVisible(showDate);
    m_dateEdit->setEnabled(showDate);  // ← ЭТОГО НЕ ХВАТАЛО!
    
    // Если выбран период — вычисляем дату автоматически
    if (!isLastN && m_comboPeriod->currentData().toString() != "custom") {
        int days = m_comboPeriod->currentData().toInt();
        QDate date = QDate::currentDate().addDays(-days);
        m_dateEdit->setDate(date);
    }
}
