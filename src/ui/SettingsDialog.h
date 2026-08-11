#pragma once

#include "core/Config.h"
#include "core/I18n.h"

#include <QWidget>

class SettingsDialog : public QWidget {
    Q_OBJECT

public:
    explicit SettingsDialog(I18n i18n, QWidget *parent = nullptr);

    void setConfig(const Config &config);

signals:
    void saved(const Config &config);

private:
    void buildUi();
    QWidget *buildGeneralPage();
    Config collectFromUi() const;
    void saveFromUi();

    I18n m_i18n;
    Config m_config;
    class HotkeyEdit *m_hotkeyEdit = nullptr;
    class QCheckBox *m_thumbnail = nullptr;
    class QPlainTextEdit *m_excluded = nullptr;
    class QButtonGroup *m_languageGroup = nullptr;
};
