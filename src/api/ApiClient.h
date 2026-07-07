#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <functional>
#include "DtoModels.h"

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);
    
    void setBaseUrl(const QString& url);
    
    // Tenders
    void getTenders(
        const std::function<void(const dto::TendersList&)>& onSuccess,
        const std::function<void(const QString&)>& onError,
        int page = 1,
        int perPage = 20,
        const QString& status = QString(),
        const QString& search = QString()
    );
    
    void getTender(
        int id,
        const std::function<void(const dto::Tender&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
    
    void createTender(
        const dto::Tender& tender,
        const std::function<void(const dto::Tender&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
    
    void updateTender(
        int id,
        const dto::Tender& tender,
        const std::function<void(const dto::Tender&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
    
    void updateTenderStatus(
        int id,
        const QString& status,
        const QString& comment,
        const std::function<void(const dto::Tender&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
	
	// Загрузка файлов для тендера
	void uploadDocuments(
		int tenderId,
		const QStringList& filePaths,
		const std::function<void(const QJsonObject&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);
    
    // Emails
    void getEmails(
        const std::function<void(const dto::EmailsList&)>& onSuccess,
        const std::function<void(const QString&)>& onError,
        int page = 1,
        int perPage = 50,
        const QString& category = QString(),
        const QString& search = QString()
    );
    
    void linkEmailToTender(
        int emailId,
        int tenderId,
        const std::function<void()>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
	
	// Обработка писем с настройками
	void processEmails(
		int limit,
		const QString& sinceDate,  // пустая строка = не использовать
		const std::function<void()>& onSuccess,
		const std::function<void(const QString&)>& onError
	);
    
    // Worker
    void triggerEmailProcessing(
        const std::function<void()>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
	
	// Проверка IMAP-подключения
	void checkImapConnection(
		const std::function<void(const QJsonObject&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);
	
	// RAG: документы
	void getDocuments(
		int tenderId,
		const std::function<void(const QJsonArray&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);
	
	void createDocument(
		int tenderId,
		const QString& docType,
		const QString& title,
		const QString& content,
		const std::function<void(const QJsonObject&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);

	void indexDocument(
		int documentId,
		const std::function<void(const QJsonObject&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);

	// RAG: поиск и запросы
	void ragQuery(
		const QString& question,
		int tenderId,  // -1 = все тендеры
		int topK,
		const std::function<void(const QJsonObject&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);

	void getRagHistory(
		int tenderId,  // -1 = все
		int limit,
		const std::function<void(const QJsonArray&)>& onSuccess,
		const std::function<void(const QString&)>& onError
	);

    // Статистика и настройки
    void getStatistics(
        const std::function<void(const QJsonObject&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );

    void pingServer(
        const std::function<void()>& onSuccess,
        const std::function<void(const QString&)>& onError
    );

signals:
    void errorOccurred(const QString& error);

private:
    QNetworkAccessManager* m_nam;
    QUrl m_baseUrl;
    
    void handleJsonResponse(
        QNetworkReply* reply,
        const std::function<void(const QJsonObject&)>& onSuccess,
        const std::function<void(const QString&)>& onError
    );
};
