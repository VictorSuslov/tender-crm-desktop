#include "MainWindow.h"
#include "TendersView.h"
#include "EmailsView.h"
#include "SettingsView.h"
#include "RagView.h" 
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_api(new ApiClient(this))
{
    setupUi();
    setupConnections();
    
    setWindowTitle("Tender CRM — Управление тендерами");
    resize(1400, 800);
    
    // Загружаем данные при старте
    m_tendersView->refresh();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    // Меню
    auto* fileMenu = menuBar()->addMenu("Файл");
    auto* refreshAction = fileMenu->addAction("Обновить");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, [this]() {
        m_tendersView->refresh();
    });
    
    auto* processAction = fileMenu->addAction("Обработать почту");
    connect(processAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage("Запуск обработки писем...");
        m_api->triggerEmailProcessing(
            [this]() {
                statusBar()->showMessage("Обработка запущена", 3000);
                // Обновим список через несколько секунд
                QTimer::singleShot(5000, this, [this]() {
                    m_tendersView->refresh();
                    m_emailsView->refresh();
                });
            },
            [this](const QString& error) {
                QMessageBox::warning(this, "Ошибка", error);
            }
        );
    });
    
    fileMenu->addSeparator();
    
    auto* quitAction = fileMenu->addAction("Выход");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    
    // Табы
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);
    
    m_tendersView = new TendersView(m_api, this);
    m_emailsView = new EmailsView(m_api, this);
    m_settingsView = new SettingsView(m_api, this);
	m_ragView = new RagView(m_api, this);
    
    m_tabs->addTab(m_tendersView, "🏆 Тендеры");
    m_tabs->addTab(m_emailsView, "📧 Письма");
    m_tabs->addTab(m_settingsView, "⚙ Настройки");
	m_tabs->addTab(m_ragView, "🤖 AI Поиск");

    // После создания m_settingsView
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        // Если переключились на вкладку настроек (индекс 2)
        if (index == 2) {
            m_settingsView->refresh();
        }
    });
    
    // Status bar
    statusBar()->showMessage("Готово");
}

void MainWindow::setupConnections() {
    connect(m_api, &ApiClient::errorOccurred, this, [this](const QString& error) {
        statusBar()->showMessage("Ошибка: " + error, 5000);
    });
}
