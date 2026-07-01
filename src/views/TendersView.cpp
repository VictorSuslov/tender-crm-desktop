#include "TendersView.h"
#include "dialogs/TenderEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

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
    TenderEditDialog dialog(m_api, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void TendersView::onEditTender() {
    auto index = m_table->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, "Информация", "Выберите тендер");
        return;
    }
    
    int id = m_model->tenderIdAt(index.row());
    TenderEditDialog dialog(m_api, this, id);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
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