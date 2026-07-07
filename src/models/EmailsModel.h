#pragma once
#include <QAbstractTableModel>
#include "api/DtoModels.h"
#include "api/ApiClient.h"

class EmailsModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColId = 0,
        ColDate,
        ColCategory,
        ColFrom,
        ColSubject,
        ColSummary,
        ColAttachments,
        ColLinked,
        ColCount
    };
    
    explicit EmailsModel(ApiClient* api, QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    
    void load(int page = 1, const QString& category = QString(), const QString& search = QString(), int perPage = 50);
    void refresh();
    
    const dto::Email& emailAt(int row) const;
    int emailIdAt(int row) const;
    
    int totalCount() const { return m_total; }
    int currentPage() const { return m_page; }
	void setEmails(const QList<dto::Email>& emails);

signals:
    void loadingStarted();
    void loadingFinished();
    void errorOccurred(const QString& error);
    void totalChanged(int total);

private:
    ApiClient* m_api;
    QList<dto::Email> m_emails;
    int m_total = 0;
    int m_page = 1;
};