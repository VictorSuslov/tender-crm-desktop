#include "EmailDetailDialog.h"
#include <QVBoxLayout>
#include <QLabel>

EmailDetailDialog::EmailDetailDialog(ApiClient* api, int emailId, QWidget* parent)
    : QDialog(parent), m_api(api), m_emailId(emailId)
{
    setWindowTitle("Детали письма");
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Детальный просмотр письма будет реализован позже"));
    resize(600, 400);
}