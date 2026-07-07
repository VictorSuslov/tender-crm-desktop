#pragma once
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class MoneyDelegate : public QStyledItemDelegate {
public:
    explicit MoneyDelegate(QObject* parent = nullptr) 
        : QStyledItemDelegate(parent) {}
    
    QString displayText(const QVariant& value, const QLocale&) const override {
        // Проверяем, что значение валидно
        if (!value.isValid()) {
            return QString();  // Пустая строка для невалидных значений
        }
        
        double num = value.toDouble();
        
        if (num <= 0) {
            return "—";
        }
        
        // Форматируем число с двумя знаками после запятой
        QString formatted = QString::number(num, 'f', 2);
        
        // Вставляем пробелы как разделители тысяч
        int dotPos = formatted.indexOf('.');
        int pos = dotPos - 3;
        while (pos > 0) {
            formatted.insert(pos, ' ');
            pos -= 3;
        }
        
        return formatted + " ₽";
    }
    
    void initStyleOption(QStyleOptionViewItem* option, 
                         const QModelIndex& index) const override {
        // Вызываем базовую реализацию для корректного фона/выделения
        QStyledItemDelegate::initStyleOption(option, index);
        
        // Выравнивание по правому краю
        option->displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
        
        // Получаем значение и форматируем
        QVariant value = index.data(Qt::EditRole);
        if (value.isValid()) {
            option->text = displayText(value, QLocale());
        }
    }
};