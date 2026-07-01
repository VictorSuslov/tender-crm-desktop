#pragma once
#include <QDialog>
#include "api/ApiClient.h"

class EmailDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit EmailDetailDialog(ApiClient* api, int emailId, QWidget* parent = nullptr);
private:
    ApiClient* m_api;
    int m_emailId;
};