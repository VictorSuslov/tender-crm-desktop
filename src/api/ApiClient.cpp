#include "ApiClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>
#include <QHttpPart>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

ApiClient::ApiClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_baseUrl("http://localhost:8000")
{
}

void ApiClient::setBaseUrl(const QString& url) {
    m_baseUrl = QUrl(url);
}

void ApiClient::handleJsonResponse(
    QNetworkReply* reply,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError, this]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString error = reply->errorString();
            // Попробуем получить детали из тела ответа
            QByteArray body = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject() && doc.object().contains("detail")) {
                error = doc.object()["detail"].toString();
            }
            onError(error);
            emit errorOccurred(error);
            return;
        }
        
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isObject()) {
            onError("Invalid JSON response");
            return;
        }
        
        onSuccess(doc.object());
    });
}

// ============ Tenders ============
void ApiClient::getTenders(
    const std::function<void(const dto::TendersList&)>& onSuccess,
    const std::function<void(const QString&)>& onError,
    int page, int perPage,
    const QString& status, const QString& search
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/tenders/");
    
    QUrlQuery query;
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("per_page", QString::number(perPage));
    if (!status.isEmpty()) query.addQueryItem("status", status);
    if (!search.isEmpty()) query.addQueryItem("search", search);
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(dto::TendersList::fromJson(obj));
        },
        onError
    );
}

void ApiClient::getTender(
    int id,
    const std::function<void(const dto::Tender&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/tenders/%1").arg(id));
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(dto::Tender::fromJson(obj));
        },
        onError
    );
}

void ApiClient::createTender(
    const dto::Tender& tender,
    const std::function<void(const dto::Tender&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/tenders/");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QByteArray data = QJsonDocument(tender.toJson()).toJson();
    QNetworkReply* reply = m_nam->post(request, data);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(dto::Tender::fromJson(obj));
        },
        onError
    );
}

void ApiClient::updateTender(
    int id,
    const dto::Tender& tender,
    const std::function<void(const dto::Tender&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/tenders/%1").arg(id));
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QByteArray data = QJsonDocument(tender.toJson()).toJson();
    QNetworkReply* reply = m_nam->put(request, data);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(dto::Tender::fromJson(obj));
        },
        onError
    );
}

void ApiClient::updateTenderStatus(
    int id,
    const QString& status,
    const QString& comment,
    const std::function<void(const dto::Tender&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/tenders/%1/status").arg(id));
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject obj;
    obj["status"] = status;
    if (!comment.isEmpty()) obj["comment"] = comment;
    
    QByteArray data = QJsonDocument(obj).toJson();
    QNetworkReply* reply = m_nam->sendCustomRequest(request, "PATCH", data);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(dto::Tender::fromJson(obj));
        },
        onError
    );
}

void ApiClient::uploadDocuments(
    int tenderId,
    const QStringList& filePaths,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    qDebug().noquote() << "\n📤 === uploadDocuments START ===";
    qDebug().noquote() << "  Tender ID:" << tenderId;
    qDebug().noquote() << "  Files:" << filePaths;
    
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/documents/upload/%1").arg(tenderId));
    qDebug().noquote() << "  URL:" << url.toString();
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    bool hasValidFiles = false;
    
    for (const QString& path : filePaths) {
        QFile* file = new QFile(path);
        if (!file->open(QIODevice::ReadOnly)) {
            qDebug().noquote() << "  ⚠️ Не удалось открыть:" << path;
            delete file;
            continue;
        }
        
        qDebug().noquote() << "  📄 Добавлен:" << QFileInfo(path).fileName() 
                           << "(" << file->size() << " bytes)";
        
        QHttpPart part;
        QString fileName = QFileInfo(path).fileName();
        part.setHeader(QNetworkRequest::ContentDispositionHeader, 
                      QString("form-data; name=\"files\"; filename=\"%1\"").arg(fileName));
        part.setBodyDevice(file);
        file->setParent(multiPart);
        multiPart->append(part);
        hasValidFiles = true;
    }
    
    if (!hasValidFiles) {
        onError("Не удалось прочитать файлы");
        return;
    }
    
    QNetworkRequest request(url);
    request.setTransferTimeout(300000); // 5 минут
    
    QNetworkReply* reply = m_nam->post(request, multiPart);
    multiPart->setParent(reply);
    
    qDebug().noquote() << "  🚀 Отправка запроса...";
    
    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError, this]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            QByteArray body = reply->readAll();
            if (!body.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(body);
                if (doc.isObject() && doc.object().contains("detail")) {
                    err = doc.object()["detail"].toString();
                }
            }
            qDebug().noquote() << "  ❌ Ошибка сети:" << err;
            onError(err);
            return;
        }
        
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qDebug().noquote() << "  ❌ Невалидный JSON ответ";
            onError("Invalid server response");
            return;
        }
        
        qDebug().noquote() << "  ✅ Успех:" << doc.object();
        onSuccess(doc.object());
    });
}

// ============ Emails ============
void ApiClient::getEmails(
    const std::function<void(const dto::EmailsList&)>& onSuccess,
    const std::function<void(const QString&)>& onError,
    int page,
    int perPage,
    const QString& category,
    const QString& search
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/emails/");
    
    QUrlQuery query;
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("per_page", QString::number(perPage));
    
    if (!category.isEmpty()) {
        query.addQueryItem("category", category);
    }
    if (!search.isEmpty()) {
        query.addQueryItem("search", search);
    }
    
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            dto::EmailsList list;
            list.total = obj["total"].toInt();
            list.page = obj["page"].toInt();
            list.per_page = obj["per_page"].toInt();
            
            QJsonArray arr = obj["items"].toArray();
            for (const auto& val : arr) {
                dto::Email email;
                QJsonObject emailObj = val.toObject();  // ⭐ Переименовали из obj в emailObj
                
                email.id = emailObj["id"].toInt();
                email.uid = emailObj["uid"].toString();
                email.from_email = emailObj["from_email"].toString();
                email.from_name = emailObj["from_name"].toString();
                email.subject = emailObj["subject"].toString();
                email.category = emailObj["category"].toString();
                email.summary = emailObj["summary"].toString();
                
                email.body_text = emailObj["body_text"].toString();
                email.body_html = emailObj["body_html"].toString();
                
                if (emailObj.contains("tender_details") && !emailObj["tender_details"].isNull()) {
                    email.tender_details = emailObj["tender_details"].toObject();
                }
                
                if (emailObj.contains("attachments_info") && !emailObj["attachments_info"].isNull()) {
                    email.attachments_info = emailObj["attachments_info"].toArray();
                }
                
                QString dateStr = emailObj["email_date"].toString();
                if (!dateStr.isEmpty()) {
                    email.email_date = QDateTime::fromString(dateStr, Qt::ISODate);
                }
                
                // ⭐ НОВОЕ: парсим привязки к тендерам
                if (emailObj.contains("linked_tenders") && emailObj["linked_tenders"].isArray()) {
                    QJsonArray linksArray = emailObj["linked_tenders"].toArray();
                    for (const QJsonValue& linkVal : linksArray) {
                        QJsonObject linkObj = linkVal.toObject();
                        dto::EmailTenderLinkInfo link;
                        link.tender_id = linkObj["tender_id"].toInt();
                        link.link_type = linkObj["link_type"].toString();
                        link.tender_name = linkObj["tender_name"].toString();
                        email.linked_tenders.append(link);
                    }
                }
                
                list.items.append(email);
            }
            onSuccess(list);
        },
        onError
    );
}

void ApiClient::linkEmailToTender(
    int emailId,
    int tenderId,
    const std::function<void()>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/emails/%1/link/%2").arg(emailId).arg(tenderId));
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->post(request, QByteArray());
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject&) { onSuccess(); },
        onError
    );
}

void ApiClient::processEmails(
    int limit,
    const QString& sinceDate,
    const std::function<void()>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/worker/process");
    
    // Добавляем query параметры
    QUrlQuery query;
    query.addQueryItem("limit", QString::number(limit));
    
    if (!sinceDate.isEmpty()) {
        query.addQueryItem("since_date", sinceDate);
    }
    
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(300000);  // 5 минут
    
    QNetworkReply* reply = m_nam->post(request, QByteArray());
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            Q_UNUSED(obj);
            onSuccess();
        },
        onError
    );
}

// ============ Worker ============
void ApiClient::triggerEmailProcessing(
    const std::function<void()>& onSuccess,
    const std::function<void(const QString&)>& onError
    ) {
    QUrl url = m_baseUrl;
    url.setPath("/api/worker/process");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Увеличиваем таймаут до 5 минут (300 секунд)
    request.setTransferTimeout(300000);  // 5 минут в миллисекундах

    QNetworkReply* reply = m_nam->post(request, QByteArray());

    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            // Логируем результат
            QString summary = obj["summary"].toString();
            qDebug() << "Обработка завершена:" << summary;
            onSuccess();
        },
        onError
    );
}

void ApiClient::checkImapConnection(
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/worker/check-imap");
    
    QNetworkRequest request(url);
    request.setTransferTimeout(30000);  // 30 секунд
    
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply, onSuccess, onError);
}

// ============ RAG: Documents ============

void ApiClient::getDocuments(
    int tenderId,
    const std::function<void(const QJsonArray&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/documents/tenders/%1").arg(tenderId));
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(obj["items"].toArray());
        },
        onError
    );
}

void ApiClient::createDocument(
    int tenderId,
    const QString& docType,
    const QString& title,
    const QString& content,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/documents/tenders/%1").arg(tenderId));
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject obj;
    obj["doc_type"] = docType;
    obj["title"] = title;
    obj["content"] = content;
    
    QByteArray data = QJsonDocument(obj).toJson();
    QNetworkReply* reply = m_nam->post(request, data);
    
    handleJsonResponse(reply, onSuccess, onError);
}

void ApiClient::indexDocument(
    int documentId,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath(QString("/api/documents/%1/index").arg(documentId));
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->post(request, QByteArray());
    
    handleJsonResponse(reply, onSuccess, onError);
}

// ============ RAG: Query ============

void ApiClient::ragQuery(
    const QString& question,
    int tenderId,
    int topK,
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/rag/query");
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject obj;
    obj["question"] = question;
    obj["top_k"] = topK;
    if (tenderId > 0) {
        obj["tender_id"] = tenderId;
    } else {
        obj["tender_id"] = QJsonValue();  // null
    }
    
    QByteArray data = QJsonDocument(obj).toJson();
    QNetworkReply* reply = m_nam->post(request, data);
    
    handleJsonResponse(reply, onSuccess, onError);
}

void ApiClient::getRagHistory(
    int tenderId,
    int limit,
    const std::function<void(const QJsonArray&)>& onSuccess,
    const std::function<void(const QString&)>& onError
) {
    QUrl url = m_baseUrl;
    url.setPath("/api/rag/history");
    
    QUrlQuery query;
    query.addQueryItem("limit", QString::number(limit));
    if (tenderId > 0) {
        query.addQueryItem("tender_id", QString::number(tenderId));
    }
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);
    
    handleJsonResponse(reply,
        [onSuccess](const QJsonObject& obj) {
            onSuccess(obj["items"].toArray());
        },
        onError
    );
}

// ============ Statistics ============

void ApiClient::getStatistics(
    const std::function<void(const QJsonObject&)>& onSuccess,
    const std::function<void(const QString&)>& onError
    ) {
    // Собираем статистику из нескольких эндпоинтов
    // Для простоты используем только documents/statistics/overview
    QUrl url = m_baseUrl;
    url.setPath("/api/documents/statistics/overview");

    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);

    handleJsonResponse(reply, onSuccess, onError);
}

void ApiClient::pingServer(
    const std::function<void()>& onSuccess,
    const std::function<void(const QString&)>& onError
    ) {
    // Пингуем сервер через получение списка тендеров (минимальный запрос)
    QUrl url = m_baseUrl;
    url.setPath("/api/tenders/");

    QUrlQuery query;
    query.addQueryItem("per_page", "1");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_nam->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError, this]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onError(reply->errorString());
            return;
        }

        onSuccess();
    });
}
