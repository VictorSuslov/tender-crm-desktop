#include "TendersModel.h"
#include <QBrush>
#include <QColor>

TendersModel::TendersModel(ApiClient* api, QObject* parent)
    : QAbstractTableModel(parent)
    , m_api(api)
{
}

int TendersModel::rowCount(const QModelIndex&) const {
    return m_tenders.size();
}

int TendersModel::columnCount(const QModelIndex&) const {
    return ColCount;
}

QVariant TendersModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_tenders.size())
        return {};
    
    const auto& t = m_tenders[index.row()];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColId: return t.id;
            case ColStatus: return t.status;
            case ColNoticeNumber: return t.notice_number;
            case ColPurchaseName: return t.purchase_name;
            case ColCustomer: return t.customer_name;
            case ColNmck:
                if (t.nmck > 0)
                    return QString("%1 %2").arg(t.nmck, 0, 'f', 2).arg(t.currency);
                return {};
            case ColDeadline:
                if (t.application_deadline.isValid())
                    return t.application_deadline.toString("dd.MM.yyyy HH:mm");
                return {};
            case ColEmails: return t.linked_emails_count;
            case ColUpdated:
                if (t.updated_at.isValid())
                    return t.updated_at.toString("dd.MM.yyyy HH:mm");
                return {};
        }
    }
    
    if (role == Qt::ForegroundRole) {
        // Цветовая индикация статуса
        if (index.column() == ColStatus) {
            if (t.status == "WON") return QBrush(QColor("#28a745"));
            if (t.status == "LOST") return QBrush(QColor("#dc3545"));
            if (t.status == "IN_PROGRESS") return QBrush(QColor("#007bff"));
            if (t.status == "SUBMITTED") return QBrush(QColor("#ffc107"));
            if (t.status == "ARCHIVED") return QBrush(QColor("#6c757d"));
        }
    }
    
    if (role == Qt::ToolTipRole) {
        if (index.column() == ColPurchaseName)
            return t.purchase_name;
    }
    
    return {};
}

QVariant TendersModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    
    switch (section) {
        case ColId: return "ID";
        case ColStatus: return "Статус";
        case ColNoticeNumber: return "№ извещения";
        case ColPurchaseName: return "Название закупки";
        case ColCustomer: return "Заказчик";
        case ColNmck: return "НМЦК";
        case ColDeadline: return "Дедлайн";
        case ColEmails: return "Писем";
        case ColUpdated: return "Обновлено";
    }
    return {};
}

void TendersModel::load(int page, const QString& status, const QString& search) {
    emit loadingStarted();
    
    m_api->getTenders(
        [this, page](const dto::TendersList& list) {
            beginResetModel();
            m_tenders = list.items;
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
        page, 50, status, search
    );
}

void TendersModel::refresh() {
    load(m_page);
}

const dto::Tender& TendersModel::tenderAt(int row) const {
    return m_tenders[row];
}

int TendersModel::tenderIdAt(int row) const {
    return m_tenders[row].id;
}