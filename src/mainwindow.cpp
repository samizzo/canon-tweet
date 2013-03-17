/* Copyright 2010 Antonie Jovanoski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include <QNetworkAccessManager>
#include <QDoubleValidator>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "oauthtwitter.h"
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_authorized = false;

    m_oauthTwitter = new OAuthTwitter(this);
    m_oauthTwitter->setNetworkAccessManager(new QNetworkAccessManager(this));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthFinished()), SLOT(xauthFinished()));
    connect(m_oauthTwitter, SIGNAL(authorizeXAuthError()), SLOT(xauthError()));

    connect(ui->authPushButton, SIGNAL(clicked()), SLOT(authorizeButtonClicked()));
    connect(ui->udpatePushButton, SIGNAL(clicked()), SLOT(updateButtonClicked()));

    m_authorized = true;
    m_oauthTwitter->setConsumerKey("8Y0v3tSsxiTVE8EPK93bKg");
    m_oauthTwitter->setConsumerSecret("38THDrK3hoFWNVYXMhS953KAqt1MgiYgxfxRvR0ROFQ");
    m_oauthTwitter->setOAuthToken("1264390094-1umU5vWcNDlFobbDAlwdJu9aRa7cW7xPGubE7wa");
    m_oauthTwitter->setOAuthTokenSecret("8ZOr4y8NMVwAWeHnNloYCSgZ3rhrLIbhn2DF7msrvwM");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::authorizeButtonClicked()
{
    m_oauthTwitter->authorizeXAuth(ui->userNameLineEdit->text(), ui->passwordLineEdit->text());
}

void MainWindow::xauthFinished()
{
    ui->statusbar->showMessage("XAuth succesfull!");
    m_authorized = true;
}

void MainWindow::xauthError()
{
    ui->statusbar->showMessage("XAuth failed");
    m_authorized = false;
}

void MainWindow::updateButtonClicked()
{
    if (m_authorized) {
        QTweetStatusUpdate *statusUpdate = new QTweetStatusUpdate(m_oauthTwitter, this);
        statusUpdate->post(ui->statusTextEdit->toPlainText());
        connect(statusUpdate, SIGNAL(postedStatus(QTweetStatus)), SLOT(postStatusFinished(QTweetStatus)));
    } else {
        ui->statusbar->showMessage("You cannot post, needs autorization!");
    }
}

void MainWindow::postStatusFinished(const QTweetStatus &status)
{
    QTweetStatusUpdate *statusUpdate = qobject_cast<QTweetStatusUpdate*>(sender());

    if (statusUpdate) {
        ui->statusbar->showMessage("Posted status with id " + QString::number(status.id()));

        statusUpdate->deleteLater();
    }
}
