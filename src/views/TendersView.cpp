#include "TendersView.h"
#include "dialogs/TenderEditDialog.h"
#include "delegates/MoneyDelegate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QCoreApplication>
#include <QDebug>

TendersView::TendersView(ApiClient* api, QWidget* parent)
    : QWidget(parent)
    , m_api(api)
{
    setupUi();
    setupConnections();
}

void TendersView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Панель инструментов
    auto* toolbar = new QHBoxLayout;
    
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Поиск по названию, номеру, заказчику...");
    m_searchEdit->setMinimumWidth(300);
    toolbar->addWidget(m_searchEdit);
    
    m_statusFilter = new QComboBox;
    m_statusFilter->addItem("Все статусы", "");
    m_statusFilter->addItem("Новые", "NEW");
    m_statusFilter->addItem("В работе", "IN_PROGRESS");
    m_statusFilter->addItem("Заявка подана", "SUBMITTED");
    m_statusFilter->addItem("Выиграны", "WON");
    m_statusFilter->addItem("Проиграны", "LOST");
    m_statusFilter->addItem("Отменены", "CANCELLED");
    m_statusFilter->addItem("В архиве", "ARCHIVED");
    toolbar->addWidget(m_statusFilter);
    
    auto* btnSearch = new QPushButton("Найти");
    toolbar->addWidget(btnSearch);
    connect(btnSearch, &QPushButton::clicked, this, &TendersView::onSearch);
    
    toolbar->addStretch();
    
    m_btnAdd = new QPushButton("➕ Добавить");
    m_btnEdit = new QPushButton("✏ Изменить");
    m_btnDelete = new QPushButton("🗑 В архив");
    m_btnRefresh = new QPushButton("🔄 Обновить");
    
    toolbar->addWidget(m_btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addWidget(m_btnRefresh);
    
    mainLayout->addLayout(toolbar);
    
    // Таблица
    m_model = new TendersModel(m_api, this);
    
    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false);
    m_table->verticalHeader()->setVisible(false);
    
    // Ширины колонок
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColPurchaseName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColId, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColStatus, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColNoticeNumber, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColCustomer, QHeaderView::Interactive);
    m_table->setColumnWidth(TendersModel::ColCustomer, 200);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColNmck, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColDeadline, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColEmails, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TendersModel::ColUpdated, QHeaderView::ResizeToContents);
    
    mainLayout->addWidget(m_table, 1);
    
    // Нижняя панель со статистикой
    auto* bottomBar = new QHBoxLayout;
    m_lblTotal = new QLabel("Всего: 0");
    bottomBar->addWidget(m_lblTotal);
    bottomBar->addStretch();
    mainLayout->addLayout(bottomBar);

    m_table->setItemDelegateForColumn(
        TendersModel::ColNmck,
        new MoneyDelegate(this)
        );
}

void TendersView::setupConnections() {
    connect(m_btnAdd, &QPushButton::clicked, this, &TendersView::onAddTender);
    connect(m_btnEdit, &QPushButton::clicked, this, &TendersView::onEditTender);
    connect(m_btnDelete, &QPushButton::clicked, this, &TendersView::onDeleteTender);
    connect(m_btnRefresh, &QPushButton::clicked, this, &TendersView::refresh);
    connect(m_table, &QTableView::doubleClicked, this, &TendersView::onRowDoubleClicked);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TendersView::onStatusFilterChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &TendersView::onSearch);
    
    connect(m_model, &TendersModel::totalChanged, this, [this](int total) {
        m_lblTotal->setText(QString("Всего: %1").arg(total));
    });
    
    connect(m_model, &TendersModel::loadingStarted, this, [this]() {
        m_lblTotal->setText("Загрузка...");
    });
    
    connect(m_model, &TendersModel::errorOccurred, this, [this](const QString& error) {
        QMessageBox::warning(this, "Ошибка", error);
    });
}

void TendersView::refresh() {
    QString status = m_statusFilter->currentData().toString();
    QString search = m_searchEdit->text();
    m_model->load(1, status, search);
}

void TendersView::onAddTender() {
    qDebug().noquote() << "\n🆕 === onAddTender START ===";
    
    TenderEditDialog dialog(m_api, this);
    int result = dialog.exec();
    
    qDebug().noquote() << "📋 dialog.exec() вернулся с кодом:" << result;
    QCoreApplication::processEvents(); // Гарантируем вывод в консоль
    
    if (result != QDialog::Accepted) {
        qDebug().noquote() << "❌ Диалог отменён";
        return;
    }
    
    int tenderId = dialog.getCreatedTenderId();
    QStringList files = dialog.getAttachedFiles();
    
    qDebug().noquote() << "✅ ID тендера:" << tenderId;
    qDebug().noquote() << "📎 Файлов:" << files.size();
    
    if (tenderId <= 0) {
        qDebug().noquote() << "⚠️ tenderId <= 0. Прерываем.";
        QMessageBox::warning(this, "Ошибка", "Тендер не был создан.");
        return;
    }
    
    if (files.isEmpty()) {
        qDebug().noquote() << "ℹ️ Файлов нет. Обновляем список.";
        refresh();
        return;
    }
    
    qDebug().noquote() << "🚀 Вызываем uploadDocuments...";
    m_api->uploadDocuments(
        tenderId,
        files,
        [this, tenderId](const QJsonObject& res) {
            int count = res["uploaded_count"].toInt();
            qDebug().noquote() << "✅ Загружено документов:" << count;
            QMessageBox::information(this, "Успех", 
                QString("Тендер создан.\nДокументов проиндексировано: %1").arg(count));
            refresh();
        },
        [this](const QString& err) {
            qDebug().noquote() << "❌ Ошибка загрузки:" << err;
            QMessageBox::warning(this, "Предупреждение", 
                "Тендер создан, но файлы не загружены:\n" + err);
            refresh();
        }
    );
    
    qDebug().noquote() << "🏁 === onAddTender END ===";
}

void TendersView::onEditTender() {
    auto index = m_table->currentIndex();
    if (!index.isValid()) {
        qDebug().noquote() << "⚠️ Нет выбранной строки";
        return;
    }
    
    int id = m_model->tenderAt(index.row()).id;
    qDebug().noquote() << "\n✏️ === onEditTender START, ID:" << id << "===";
    
    TenderEditDialog dialog(m_api, this, id);
    int result = dialog.exec();
    
    qDebug().noquote() << "📋 dialog.exec() вернулся с кодом:" << result;
    
    if (result != QDialog::Accepted) {
        qDebug().noquote() << "❌ Диалог отменён";
        return;
    }
    
    QStringList files = dialog.getAttachedFiles();
    qDebug().noquote() << "📎 Файлов для загрузки:" << files.size();
    
    if (files.isEmpty()) {
        qDebug().noquote() << "ℹ️ Файлов нет, просто обновляем список";
        refresh();
        return;
    }
    
    qDebug().noquote() << "🚀 Вызываем uploadDocuments для тендера ID:" << id;
    m_api->uploadDocuments(
        id,
        files,
        [this, id](const QJsonObject& res) {
            int count = res["uploaded_count"].toInt();
            qDebug().noquote() << "✅ Загружено документов:" << count;
            QMessageBox::information(this, "Успех", 
                QString("Тендер обновлён.\nДокументов проиндексировано: %1").arg(count));
            refresh();
        },
        [this](const QString& err) {
            qDebug().noquote() << "❌ Ошибка загрузки:" << err;
            QMessageBox::warning(this, "Предупреждение", 
                "Тендер обновлён, но файлы не загружены:\n" + err);
            refresh();
        }
    );
    
    qDebug().noquote() << "🏁 === onEditTender END ===";
}

void TendersView::onDeleteTender() {
    auto index = m_table->currentIndex();
    if (!index.isValid()) return;
    
    int id = m_model->tenderIdAt(index.row());
    const auto& t = m_model->tenderAt(index.row());
    
    auto result = QMessageBox::question(
        this,
        "Архивирование",
        QString("Переместить тендер \"%1\" в архив?").arg(t.purchase_name.left(80))
    );
    
    if (result != QMessageBox::Yes) return;
    
    m_api->updateTenderStatus(
        id, "ARCHIVED", "",
        [this](const dto::Tender&) {
            refresh();
        },
        [this](const QString& error) {
            QMessageBox::warning(this, "Ошибка", error);
        }
    );
}

void TendersView::onRowDoubleClicked(const QModelIndex&) {
    onEditTender();
}

void TendersView::onSearch() {
    refresh();
}

void TendersView::onStatusFilterChanged() {
    refresh();
}

void TendersView::onPageChanged() {
    // TODO: пагинация
}
