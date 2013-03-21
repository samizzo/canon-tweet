#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include "qtweetnetbase.h"

class OAuthTwitter;
class QTweetStatus;

class MainWindow : public QObject
{
    Q_OBJECT

public:
    explicit MainWindow();

signals:
    void finished();

public slots:
    void run();

private slots:
    void postStatusFinished(const QTweetStatus& status);
    void postStatusError(QTweetNetBase::ErrorCode errorCode, QString errorMsg);

private:
    void postMessage(const QString& message);
    void postMessageWithImage(const QString& message, const QString& imagePath);
    void showUsage();
    void quit();

    OAuthTwitter *m_oauthTwitter;
    bool m_authorized;
};

#endif // MAINWINDOW_H
