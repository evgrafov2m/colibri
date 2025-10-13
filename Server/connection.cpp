#include "connection.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

Connection::Connection(const QString &clientId, QTcpSocket *socket, QObject *parent)
    : QObject(parent), socket_(socket), cid_(clientId)
{
  socket_->setParent(this);
  connect(socket_, &QTcpSocket::readyRead, this, &Connection::OnReadyRead);
  connect(socket_, &QTcpSocket::disconnected, this, &Connection::Disconnected);
  connect(socket_, &QAbstractSocket::errorOccurred, this, &Connection::OnSocketError);
}

Connection::~Connection()
{
  if(socket_) {
    socket_->close();
    socket_->deleteLater();
  }
}

void Connection::OnReadyRead()
{
  if(!socket_)
    return;

  QDataStream in(socket_);
  in.setVersion(QDataStream::Qt_6_0);

  forever{
    in.startTransaction();

    QByteArray jsonData;
    in >> jsonData;

    if(!in.commitTransaction())
      break;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(jsonData, &err);

    if (err.error != QJsonParseError::NoError) {
      emit ErrorOccurred(cid_, QStringLiteral("JSON parse error: %1")
                             .arg(err.errorString()));
      continue;
    }

    if(!doc.isObject())
      continue;

    emit JsonObject(doc.object());
  }
}

void Connection::SendJson(const QJsonObject &obj)
{
  if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState)
    return;

  QByteArray data;
  QDataStream out(&data, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_6_0);

  out << QJsonDocument(obj).toJson(QJsonDocument::Compact);
  socket_->write(data);
}

void Connection::OnSocketError(QAbstractSocket::SocketError)
{
  if(!socket_)
    return;

  emit ErrorOccurred(cid_, socket_->errorString());
}

void Connection::DisconnectSocket()
{
  if(socket_)
    socket_->disconnectFromHost();
}
