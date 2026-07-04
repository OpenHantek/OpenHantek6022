// SPDX-License-Identifier: GPL-2.0-or-later

#include "remoteserver.h"

#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

RemoteServer::RemoteServer( quint16 port, Executor executor, QObject *parent )
    : QObject( parent ), server( new QTcpServer( this ) ), executor( std::move( executor ) ) {
    connect( server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection );
    if ( !server->listen( QHostAddress::LocalHost, port ) )
        qWarning() << "RemoteServer: cannot listen on port" << port << ":" << server->errorString();
    else
        qInfo() << "RemoteServer: listening on 127.0.0.1 port" << server->serverPort();
}


bool RemoteServer::isListening() const { return server->isListening(); }


quint16 RemoteServer::serverPort() const { return server->serverPort(); }


void RemoteServer::onNewConnection() {
    while ( QTcpSocket *client = server->nextPendingConnection() ) {
        connect( client, &QTcpSocket::readyRead, this, [ this, client ]() { onReadyRead( client ); } );
        connect( client, &QTcpSocket::disconnected, client, &QObject::deleteLater );
    }
}


void RemoteServer::onReadyRead( QTcpSocket *client ) {
    while ( client->canReadLine() ) {
        const QString line = QString::fromUtf8( client->readLine() ).trimmed();
        if ( line.isEmpty() )
            continue;
        QString reply = executor ? executor( line ) : QStringLiteral( "ERR no executor" );
        reply.append( '\n' );
        client->write( reply.toUtf8() );
    }
    client->flush();
}
