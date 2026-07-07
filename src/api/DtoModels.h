#pragma once
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <optional>

namespace dto {

// ============ Tender ============
struct Tender {
    int id = 0;
    QString notice_number;
    QString lot_number;
    QString purchase_name;
    QString customer_name;
    QString etp_url;
    double nmck = 0.0;
    QString currency = "RUB";
    QDate publication_date;
    QDateTime application_deadline;
    QDateTime auction_date;
    QDate contract_deadline;
    QString status = "NEW";
    QString result;
    int responsible_user_id = 0;
    int created_by = 0;
    QString notes;
    QDateTime created_at;
    QDateTime updated_at;
    int linked_emails_count = 0;
    
    static Tender fromJson(const QJsonObject& obj) {
        Tender t;
        t.id = obj["id"].toInt();
        t.notice_number = obj["notice_number"].toString();
        t.lot_number = obj["lot_number"].toString();
        t.purchase_name = obj["purchase_name"].toString();
        t.customer_name = obj["customer_name"].toString();
        t.etp_url = obj["etp_url"].toString();

        QJsonValue nmckValue = obj["nmck"];
        if (nmckValue.isString()) {
            t.nmck = nmckValue.toString().toDouble();
        } else {
            t.nmck = nmckValue.toDouble();
        }

        t.currency = obj["currency"].toString("RUB");
        t.status = obj["status"].toString("NEW");
        t.result = obj["result"].toString();
        t.responsible_user_id = obj["responsible_user_id"].toInt();
        t.notes = obj["notes"].toString();
        t.linked_emails_count = obj["linked_emails_count"].toInt();
        
        if (obj.contains("created_at") && !obj["created_at"].isNull())
            t.created_at = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
        if (obj.contains("updated_at") && !obj["updated_at"].isNull())
            t.updated_at = QDateTime::fromString(obj["updated_at"].toString(), Qt::ISODate);
        if (obj.contains("application_deadline") && !obj["application_deadline"].isNull())
            t.application_deadline = QDateTime::fromString(obj["application_deadline"].toString(), Qt::ISODate);
        
        return t;
    }
    
    QJsonObject toJson() const {
        QJsonObject obj;
        if (!notice_number.isEmpty()) obj["notice_number"] = notice_number;
        if (!lot_number.isEmpty()) obj["lot_number"] = lot_number;
        obj["purchase_name"] = purchase_name;
        if (!customer_name.isEmpty()) obj["customer_name"] = customer_name;
        if (!etp_url.isEmpty()) obj["etp_url"] = etp_url;
        if (nmck > 0) obj["nmck"] = nmck;
        if (currency != "RUB") obj["currency"] = currency;
        if (!notes.isEmpty()) obj["notes"] = notes;
        return obj;
    }
};

struct TendersList {
    QList<Tender> items;
    int total = 0;
    int page = 1;
    int per_page = 20;
    
    static TendersList fromJson(const QJsonObject& obj) {
        TendersList list;
        list.total = obj["total"].toInt();
        list.page = obj["page"].toInt();
        list.per_page = obj["per_page"].toInt();
        
        for (const auto& item : obj["items"].toArray()) {
            list.items.append(Tender::fromJson(item.toObject()));
        }
        return list;
    }
};

struct EmailTenderLinkInfo {
    int tender_id = 0;
    QString link_type;
    QString tender_name;
};

// ============ Email ============
struct Email {
    int id = 0;
    QString uid;
    QString from_email;
    QString from_name;
    QString subject;
    QString category;
    QString summary;
    QString body_text;
    QString body_html;
    QDateTime email_date;
    QDateTime received_at;
    QJsonArray attachments_info;
    QJsonObject tender_details;
    QList<EmailTenderLinkInfo> linked_tenders;  // ⭐ Только один тип
    
    static Email fromJson(const QJsonObject& obj) {
        Email e;
        e.id = obj["id"].toInt();
        e.uid = obj["uid"].toString();
        e.from_email = obj["from_email"].toString();
        e.from_name = obj["from_name"].toString();
        e.subject = obj["subject"].toString();
        e.category = obj["category"].toString();
        e.summary = obj["summary"].toString();
        e.body_text = obj["body_text"].toString();
        e.body_html = obj["body_html"].toString();
        e.attachments_info = obj["attachments_info"].toArray();
        e.tender_details = obj["tender_details"].toObject();
        
        if (obj.contains("email_date") && !obj["email_date"].isNull())
            e.email_date = QDateTime::fromString(obj["email_date"].toString(), Qt::ISODate);
        
        // ⭐ Парсим привязки к тендерам
        if (obj.contains("linked_tenders") && obj["linked_tenders"].isArray()) {
            QJsonArray linksArray = obj["linked_tenders"].toArray();
            for (const QJsonValue& linkVal : linksArray) {
                QJsonObject linkObj = linkVal.toObject();
                dto::EmailTenderLinkInfo link;
                link.tender_id = linkObj["tender_id"].toInt();
                link.link_type = linkObj["link_type"].toString();
                link.tender_name = linkObj["tender_name"].toString();
                e.linked_tenders.append(link);
            }
        }
        
        return e;
    }
};

struct EmailsList {
    QList<Email> items;
    int total = 0;
    int page = 1;
    int per_page = 50;
    
    static EmailsList fromJson(const QJsonObject& obj) {
        EmailsList list;
        list.total = obj["total"].toInt();
        list.page = obj["page"].toInt();
        list.per_page = obj["per_page"].toInt();
        
        for (const auto& item : obj["items"].toArray()) {
            list.items.append(Email::fromJson(item.toObject()));
        }
        return list;
    }
};

} // namespace dto
