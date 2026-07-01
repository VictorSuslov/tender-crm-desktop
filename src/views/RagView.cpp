#include "RagView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QSpinBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

RagView::RagView(ApiClient* api, QWidget* parent)
    : QWidget(parent)
    , m_api(api)
{
    setupUi();
    setupConnections();
    loadTenders();
}

void RagView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // ===== Верхняя панель: выбор тендера и ввод вопроса =====
    auto* topGroup = new QGroupBox("🤖 Задать вопрос");
    auto* topLayout = new QVBoxLayout(topGroup);
    
    // Строка выбора тендера
    auto* tenderLayout = new QHBoxLayout;
    tenderLayout->addWidget(new QLabel("Тендер:"));
    
    m_comboTenders = new QComboBox;
    m_comboTenders->setMinimumWidth(400);
    m_comboTenders->addItem("📂 Все тендеры", -1);
    tenderLayout->addWidget(m_comboTenders, 1);
    topLayout->addLayout(tenderLayout);
    
    // Строка ввода вопроса
    auto* queryLayout = new QHBoxLayout;
    
    m_editQuestion = new QLineEdit;
    m_editQuestion->setPlaceholderText("Введите вопрос... (например: 'Какая НМЦК контракта?')");
    m_editQuestion->setMinimumWidth(400);
    queryLayout->addWidget(m_editQuestion, 1);
    
    queryLayout->addWidget(new QLabel("Источников:"));
    m_spinTopK = new QSpinBox;
    m_spinTopK->setRange(1, 20);
    m_spinTopK->setValue(5);
    m_spinTopK->setMaximumWidth(60);
    queryLayout->addWidget(m_spinTopK);
    
    m_btnAsk = new QPushButton("🔍 Задать вопрос");
    m_btnAsk->setStyleSheet("background-color: #28a745; color: white; padding: 8px 16px; font-weight: bold;");
    queryLayout->addWidget(m_btnAsk);
    
    topLayout->addLayout(queryLayout);
    
    mainLayout->addWidget(topGroup);
    
    // ===== Центральная область: ответ и источники =====
    auto* splitter = new QSplitter(Qt::Horizontal);
    
    // Левая часть: ответ LLM
    auto* answerGroup = new QGroupBox("💬 Ответ");
    auto* answerLayout = new QVBoxLayout(answerGroup);
    
    m_txtAnswer = new QTextEdit;
    m_txtAnswer->setReadOnly(true);
    m_txtAnswer->setPlaceholderText("Здесь появится ответ от AI-ассистента...");
    answerLayout->addWidget(m_txtAnswer);
    
    splitter->addWidget(answerGroup);
    
    // Правая часть: источники
    auto* sourcesGroup = new QGroupBox("📚 Источники информации");
    auto* sourcesLayout = new QVBoxLayout(sourcesGroup);
    
    m_lstSources = new QListWidget;
    m_lstSources->setAlternatingRowColors(true);
    sourcesLayout->addWidget(m_lstSources);
    
    splitter->addWidget(sourcesGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    
    mainLayout->addWidget(splitter, 1);
    
    // ===== Нижняя панель: история запросов =====
    auto* historyGroup = new QGroupBox("📜 История запросов");
    auto* historyLayout = new QVBoxLayout(historyGroup);
    
    m_lstHistory = new QListWidget;
    m_lstHistory->setMaximumHeight(150);
    m_lstHistory->setAlternatingRowColors(true);
    historyLayout->addWidget(m_lstHistory);
    
    auto* historyBtnLayout = new QHBoxLayout;
    m_btnRefreshHistory = new QPushButton("🔄 Обновить историю");
    historyBtnLayout->addWidget(m_btnRefreshHistory);
    historyBtnLayout->addStretch();
    historyLayout->addLayout(historyBtnLayout);
    
    mainLayout->addWidget(historyGroup);
}

void RagView::setupConnections() {
    connect(m_btnAsk, &QPushButton::clicked, this, &RagView::onAskQuestion);
    connect(m_editQuestion, &QLineEdit::returnPressed, this, &RagView::onAskQuestion);
    connect(m_comboTenders, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RagView::onTenderChanged);
    connect(m_lstHistory, &QListWidget::itemClicked, this, &RagView::onHistoryItemClicked);
    connect(m_btnRefreshHistory, &QPushButton::clicked, this, &RagView::onLoadHistory);
}

void RagView::loadTenders() {
    m_api->getTenders(
        [this](const dto::TendersList& list) {
            m_comboTenders->clear();
            m_comboTenders->addItem("📂 Все тендеры", -1);
            
            m_tenders.clear();
            for (const auto& t : list.items) {
                QString label = QString("[%1] %2").arg(t.id).arg(t.purchase_name.left(60));
                m_comboTenders->addItem(label, t.id);
                m_tenders.append({t.id, t.purchase_name});
            }
            
            // Загружаем историю после загрузки тендеров
            onLoadHistory();
        },
        [this](const QString& error) {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить список тендеров: " + error);
        }
    );
}

void RagView::onAskQuestion() {
    QString question = m_editQuestion->text().trimmed();
    if (question.isEmpty()) {
        QMessageBox::information(this, "Внимание", "Введите вопрос");
        return;
    }
    
    int tenderId = m_comboTenders->currentData().toInt();
    int topK = m_spinTopK->value();
    
    // Блокируем кнопку и показываем индикатор
    m_btnAsk->setEnabled(false);
    m_btnAsk->setText("⏳ Думаю...");
    m_txtAnswer->setPlainText("Поиск информации и генерация ответа...");
    m_lstSources->clear();
    
    m_api->ragQuery(
        question,
        tenderId,
        topK,
        [this](const QJsonObject& result) {
            displayAnswer(result);
            displaySources(result["sources"].toArray());
            
            m_btnAsk->setEnabled(true);
            m_btnAsk->setText("🔍 Задать вопрос");
            
            // Обновляем историю
            onLoadHistory();
        },
        [this](const QString& error) {
            m_txtAnswer->setPlainText("Ошибка: " + error);
            m_btnAsk->setEnabled(true);
            m_btnAsk->setText("🔍 Задать вопрос");
            QMessageBox::warning(this, "Ошибка", error);
        }
    );
}

void RagView::displayAnswer(const QJsonObject& result) {
    QString answer = result["answer"].toString();
    int processingTime = result["processing_time_ms"].toInt();
    int sourcesCount = result["sources"].toArray().size();
    
    QString html = "<div style='font-size: 14px; line-height: 1.6;'>";
    html += answer.toHtmlEscaped().replace("\n", "<br>");
    html += "</div>";
    html += "<hr>";
    html += "<div style='color: #666; font-size: 12px;'>";
    html += QString("⏱ Время обработки: %1 мс | 📚 Источников: %2")
        .arg(processingTime).arg(sourcesCount);
    html += "</div>";
    
    m_txtAnswer->setHtml(html);
}

void RagView::displaySources(const QJsonArray& sources) {
    m_lstSources->clear();
    
    for (const auto& src : sources) {
        QJsonObject obj = src.toObject();
        
        QString title = obj["document_title"].toString();
        QString docType = obj["doc_type"].toString();
        double similarity = obj["similarity"].toDouble();
        QString content = obj["content"].toString().left(100) + "...";
        
        QString icon;
        if (docType == "TENDER_APPLICATION") icon = "📋";
        else if (docType == "SUPPLEMENT") icon = "📝";
        else if (docType == "EMAIL") icon = "📧";
        else icon = "📄";
        
        QString text = QString("%1 %2\n   Сходство: %3% | %4")
            .arg(icon)
            .arg(title)
            .arg(similarity * 100, 0, 'f', 1)
            .arg(content);
        
        auto* item = new QListWidgetItem(text);
        item->setToolTip(content);
        item->setData(Qt::UserRole, obj["document_id"].toInt());
        m_lstSources->addItem(item);
    }
}

void RagView::onTenderChanged() {
    onLoadHistory();
}

void RagView::onLoadHistory() {
    int tenderId = m_comboTenders->currentData().toInt();
    
    m_api->getRagHistory(
        tenderId,
        20,
        [this](const QJsonArray& items) {
            m_lstHistory->clear();
            
            for (const auto& item : items) {
                QJsonObject obj = item.toObject();
                
                QString query = obj["query"].toString();
                QString date = obj["created_at"].toString().left(19).replace("T", " ");
                int procTime = obj["processing_time_ms"].toInt();
                
                QString text = QString("[%1] %2 (%3 мс)")
                    .arg(date)
                    .arg(query.left(60))
                    .arg(procTime);
                
                auto* listItem = new QListWidgetItem(text);
                listItem->setData(Qt::UserRole, query);
                m_lstHistory->addItem(listItem);
            }
        },
        [](const QString&) {
            // Тихо игнорируем ошибки истории
        }
    );
}

void RagView::onHistoryItemClicked(QListWidgetItem* item) {
    QString query = item->data(Qt::UserRole).toString();
    m_editQuestion->setText(query);
}

void RagView::refresh() {
    loadTenders();
}