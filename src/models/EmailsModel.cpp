#include "EmailsModel.h"
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QIcon>

EmailsModel::EmailsModel(ApiClient* api, QObject* parent)
    : QAbstractTableModel(parent)
    , m_api(api)
{
}

int EmailsModel::rowCount(const QModelIndex&) const {
    return m_emails.size();
}

int EmailsModel::columnCount(const QModelIndex&) const {
    return ColCount;
}

QVariant EmailsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_emails.size())
        return {};
    
    const auto& e = m_emails[index.row()];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColId: return e.id;
            case ColDate:
                if (e.email_date.isValid())
                    return e.email_date.toString("dd.MM.yyyy HH:mm");
                return {};
            case ColCategory: return e.category;
            case ColFrom:
                if (!e.from_name.isEmpty())
                    return QString("%1 <%2>").arg(e.from_name, e.from_email);
                return e.from_email;
            case ColSubject: {
                // ⭐ Добавляем иконку привязки к теме
                QString subject = e.subject.isEmpty() ? "(без темы)" : e.subject;
                if (!e.linked_tenders.isEmpty()) {
                    return QString("🔗 %1").arg(subject);
                }
                return subject;
            }
            case ColSummary: return e.summary;
            case ColAttachments: return e.attachments_info.size();
            case ColLinked: {
                // ⭐ Показываем названия тендеров через запятую
                if (e.linked_tenders.isEmpty()) {
                    return "—";
                }
                QStringList tenderNames;
                for (const auto& link : e.linked_tenders) {
                    tenderNames.append(link.tender_name);
                }
                return tenderNames.join(", ");
            }
        }
    }
    
    if (role == Qt::ForegroundRole) {
        if (index.column() == ColCategory) {
            if (e.category == "TENDER") return QBrush(QColor("#28a745"));
            if (e.category == "SPAM") return QBrush(QColor("#dc3545"));
            if (e.category == "GENERAL") return QBrush(QColor("#007bff"));
            if (e.category == "EMPTY") return QBrush(QColor("#6c757d"));
        }
        
        // ⭐ Синий текст для колонки "Тендер"
        if (index.column() == ColLinked && !e.linked_tenders.isEmpty()) {
            return QBrush(QColor("#0066cc"));
        }
    }
    
    // ⭐ НОВОЕ: Цветовая индикация фона строк
    if (role == Qt::BackgroundRole) {
        if (e.category == "TENDER") {
            if (!e.linked_tenders.isEmpty()) {
                return QBrush(QColor(200, 255, 200));  // Зелёный — привязан
            }
            return QBrush(QColor(255, 255, 200));  // Жёлтый — не привязан
        }
        if (e.category == "SPAM") {
            return QBrush(QColor(255, 220, 220));  // Красный
        }
    }
    
    if (role == Qt::ToolTipRole) {
        if (index.column() == ColSubject && !e.subject.isEmpty())
            return e.subject;
        if (index.column() == ColSummary && !e.summary.isEmpty())
            return e.summary;
        // ⭐ Подсказка для колонки "Тендер"
        if (index.column() == ColLinked && !e.linked_tenders.isEmpty()) {
            QStringList details;
            for (const auto& link : e.linked_tenders) {
                details.append(QString("%1 (ID: %2, тип: %3)")
                    .arg(link.tender_name)
                    .arg(link.tender_id)
                    .arg(link.link_type));
            }
            return details.join("\n");
        }
    }
    
    if (role == Qt::FontRole) {
        // ⭐ Жирный шрифт для темы привязанных писем
        if (index.column() == ColSubject && !e.linked_tenders.isEmpty()) {
            QFont f;
            f.setBold(true);
            return f;
        }
    }
    
    return {};
}

QVariant EmailsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    
    switch (section) {
        case ColId: return "ID";
        case ColDate: return "Дата";
        case ColCategory: return "Категория";
        case ColFrom: return "Отправитель";
        case ColSubject: return "Тема";
        case ColSummary: return "Резюме";
        case ColAttachments: return "📎";
        case ColLinked: return "🔗 Тендер";  // ⭐ Более понятный заголовок
    }
    return {};
}

void EmailsModel::load(int page, const QString& category, const QString& search, int perPage) {
    m_page = page;  // ⭐ Сохраняем текущую страницу для refresh()
    
    m_api->getEmails(
        [this](const dto::EmailsList& list) {
            beginResetModel();
            m_emails = list.items;
            endResetModel();
            emit totalChanged(list.total);
        },
        [this](const QString& error) {
            emit errorOccurred(error);
        },
        page,
        perPage,
        category,
        search
    );
}

void EmailsModel::refresh() {
    load(m_page);
}

const dto::Email& EmailsModel::emailAt(int row) const {
    return m_emails[row];
}

int EmailsModel::emailIdAt(int row) const {
    return m_emails[row].id;
}

void EmailsModel::setEmails(const QList<dto::Email>& emails) {
    beginResetModel();
    m_emails = emails;
    endResetModel();
}