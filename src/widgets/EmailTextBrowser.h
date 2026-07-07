#pragma once

#include <QTextBrowser>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCache>
#include <QUrl>

/**
 * @brief Кастомный QTextBrowser для отображения HTML-писем с изображениями.
 * 
 * Автоматически загружает внешние изображения и кэширует их.
 * Поддерживает data: URI (встроенные base64-изображения).
 */
class EmailTextBrowser : public QTextBrowser {
    Q_OBJECT

public:
    explicit EmailTextBrowser(QWidget* parent = nullptr);
    ~EmailTextBrowser();
    
    /**
     * @brief Установить HTML-контент письма.
     * Автоматически запустит загрузку изображений.
     */
    void setEmailHtml(const QString& html);

protected:
    QVariant loadResource(int type, const QUrl& name) override;

private slots:
    void onImageDownloaded();

private:
    QNetworkAccessManager* m_nam;
    QCache<QString, QByteArray> m_imageCache;
    QSet<QString> m_pendingUrls;  // URL, которые уже запрашиваем
    
    static const int MAX_CACHE_SIZE = 50 * 1024 * 1024;  // 50 MB кэш
    static const int TIMEOUT_MS = 10000;  // 10 секунд таймаут
};