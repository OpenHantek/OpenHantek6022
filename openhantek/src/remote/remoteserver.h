// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <functional>

class QTcpServer;
class QTcpSocket;

/// \brief Simple line based remote control server (SCPI style).
/// Listens on localhost only, executes each received text line
/// via the supplied executor callback (runs in the GUI thread)
/// and sends the reply back, terminated by a newline.
class RemoteServer : public QObject {
    Q_OBJECT

  public:
    using Executor = std::function< QString( const QString & ) >;

    RemoteServer( quint16 port, Executor executor, QObject *parent = nullptr );
    bool isListening() const;
    quint16 serverPort() const;

  private:
    void onNewConnection();
    void onReadyRead( QTcpSocket *client );

    QTcpServer *server;
    Executor executor;
};
