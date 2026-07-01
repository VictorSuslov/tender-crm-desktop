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

    // ===== Группа: Обработка почты =====
    auto* processingGroup = new QGroupBox("📧 Обработка почты");
    auto* processingLayout = new QVBoxLayout(processingGroup);

    auto* intervalLayout = new QHBoxLayout;
    intervalLayout->addWidget(new QLabel("Интервал проверки:"));

    m_spinInterval = new QSpinBox;
    m_spinInterval->setRange(1, 60);
    m_spinInterval->setValue(5);
    m_spinInterval->setSuffix(" мин");
    intervalLayout->addWidget(m_spinInterval);
    intervalLayout->addStretch();

    m_chkAutoRefresh = new QCheckBox("Автообновление статистики");
    m_chkAutoRefresh->setChecked(true);
    intervalLayout->addWidget(m_chkAutoRefresh);

    processingLayout->addLayout(intervalLayout);

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
    m_lblProcessStatus->setText("Запуск обработки писем. Это может занять 2-5 минут...");
    m_lblProcessStatus->setStyleSheet("color: #007bff;");

    m_api->triggerEmailProcessing(
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
}

void SettingsView::onRefreshStatistics() {
    loadStatistics();
}
