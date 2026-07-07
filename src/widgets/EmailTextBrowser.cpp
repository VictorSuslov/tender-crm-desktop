#include "EmailTextBrowser.h"
#include <QBuffer>
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>

EmailTextBrowser::EmailTextBrowser(QWidget* parent)
    : QTextBrowser(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_imageCache(MAX_CACHE_SIZE)
{
    // Настройки
    setOpenExternalLinks(true);
    setReadOnly(true);
    
    // Соединяем сигнал завершения загрузки
    connect(m_nam, &QNetworkAccessManager::finished, this, &EmailTextBrowser::onImageDownloaded);
}

EmailTextBrowser::~EmailTextBrowser() = default;

void EmailTextBrowser::setEmailHtml(const QString& html) {
    // Очищаем кэш запросов для нового письма
    m_pendingUrls.clear();
    
    // Устанавливаем HTML
    setHtml(html);
}

QVariant EmailTextBrowser::loadResource(int type, const QUrl& url) {
    // Обрабатываем только изображения
    if (type != QTextDocument::ImageResource) {
        return QTextBrowser::loadResource(type, url);
    }
    
    QString urlString = url.toString();
    
    // 1. Проверяем кэш
    if (QByteArray* cached = m_imageCache.object(urlString)) {
        QImage img;
        img.loadFromData(*cached);
        if (!img.isNull()) {
            return img;
        }
    }
    
    // 2. data: URI — встроенное изображение
    if (url.scheme() == "data") {
        // Формат: data:image/png;base64,iVBORw0KGgo...
        QString dataStr = urlString.mid(5);  // Убираем "data:"
        int commaPos = dataStr.indexOf(',');
        if (commaPos > 0) {
            QString meta = dataStr.left(commaPos);
            QString data = dataStr.mid(commaPos + 1);
            
            QByteArray bytes;
            if (meta.contains("base64")) {
                bytes = QByteArray::fromBase64(data.toUtf8());
            } else {
                bytes = QByteArray::fromPercentEncoding(data.toUtf8());
            }
            
            // Сохраняем в кэш
            m_imageCache.insert(urlString, new QByteArray(bytes), bytes.size());
            
            QImage img;
            img.loadFromData(bytes);
            if (!img.isNull()) {
                return img;
            }
        }
        return QVariant();
    }
    
    // 3. file:// URI — локальный файл
    if (url.scheme() == "file") {
        QString path = url.toLocalFile();
        if (QFileInfo::exists(path)) {
            QImage img(path);
            if (!img.isNull()) {
                return img;
            }
        }
        return QVariant();
    }
    
    // 4. http:// или https:// — скачиваем асинхронно
    if (url.scheme() == "http" || url.scheme() == "https") {
        // Если уже запрашиваем — не дублируем
        if (m_pendingUrls.contains(urlString)) {
            return QVariant();
        }
        
        m_pendingUrls.insert(urlString);
        
        QNetworkRequest request(url);
		request.setTransferTimeout(TIMEOUT_MS);
		request.setHeader(QNetworkRequest::UserAgentHeader, 
						 "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

		// Форсируем HTTP/1.1 для совместимости с серверами, не поддерживающими HTTP/2
		request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

		// Referer для некоторых серверов
		request.setRawHeader("Referer", url.toEncoded());

		m_nam->get(request);
        
        // Возвращаем пустое изображение-заглушку
        QImage placeholder(1, 1, QImage::Format_ARGB32);
        placeholder.fill(Qt::transparent);
        return placeholder;
    }
    
    return QTextBrowser::loadResource(type, url);
}

void EmailTextBrowser::onImageDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QUrl url = reply->url();
    QString urlString = url.toString();
    
    // Удаляем из списка ожидающих
    m_pendingUrls.remove(urlString);
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        
        // Определяем MIME-тип из заголовка
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        
        // Проверяем, что это изображение
        bool isImage = contentType.startsWith("image/") || 
                      QImageReader::supportedMimeTypes().contains(contentType.toUtf8());
        
        if (isImage || !data.isEmpty()) {
            // Проверяем, что это действительно изображение
            QImage testImg;
            if (testImg.loadFromData(data)) {
                // Сохраняем в кэш
                m_imageCache.insert(urlString, new QByteArray(data), data.size());
                
                // Перерисовываем документ для отображения изображения
                document()->setHtml(document()->toHtml());
                
                qDebug() << "✓ Изображение загружено:" << urlString 
                         << "(" << data.size() << "bytes)";
            }
        }
    } else {
        qDebug() << "✗ Не удалось загрузить изображение:" << urlString 
                 << "-" << reply->errorString();
    }
    
    reply->deleteLater();
}