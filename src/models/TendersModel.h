#pragma once
#include <QAbstractTableModel>
#include "api/DtoModels.h"
#include "api/ApiClient.h"

class TendersModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColId = 0,
        ColStatus,
        ColNoticeNumber,
        ColPurchaseName,
        ColCustomer,
        ColNmck,
        ColDeadline,
        ColEmails,
        ColUpdated,
        ColCount
    };
    
    explicit TendersModel(ApiClient* api, QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    
    void load(int page = 1, const QString& status = QString(), const QString& search = QString());
    void refresh();
    
    const dto::Tender& tenderAt(int row) const;
    int tenderIdAt(int row) const;
    
    int totalCount() const { return m_total; }
    int currentPage() const { return m_page; }

signals:
    void loadingStarted();
    void loadingFinished();
    void errorOccurred(const QString& error);
    void totalChanged(int total);

private:
    ApiClient* m_api;
    QList<dto::Tender> m_tenders;
    int m_total = 0;
    int m_page = 1;
};