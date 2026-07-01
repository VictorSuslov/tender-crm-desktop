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
            case ColSubject: return e.subject.isEmpty() ? "(без темы)" : e.subject;
            case ColSummary: return e.summary;
            case ColAttachments: return e.attachments_info.size();
            case ColLinked: return e.linked_tenders.size();
        }
    }
    
    if (role == Qt::ForegroundRole) {
        if (index.column() == ColCategory) {
            if (e.category == "TENDER") return QBrush(QColor("#28a745"));
            if (e.category == "SPAM") return QBrush(QColor("#dc3545"));
            if (e.category == "GENERAL") return QBrush(QColor("#007bff"));
            if (e.category == "EMPTY") return QBrush(QColor("#6c757d"));
        }
    }
    
    if (role == Qt::ToolTipRole) {
        if (index.column() == ColSubject && !e.subject.isEmpty())
            return e.subject;
        if (index.column() == ColSummary && !e.summary.isEmpty())
            return e.summary;
    }
    
    if (role == Qt::FontRole) {
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
        case ColLinked: return "🔗";
    }
    return {};
}

void EmailsModel::load(int page, const QString& category, const QString& search) {
    emit loadingStarted();
    
    m_api->getEmails(
        [this, page](const dto::EmailsList& list) {
            beginResetModel();
            m_emails = list.items;
            m_total = list.total;
            m_page = page;
            endResetModel();
            emit totalChanged(m_total);
            emit loadingFinished();
        },
        [this](const QString& error) {
            emit errorOccurred(error);
            emit loadingFinished();
        },
        page, 50, category, search
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
