#pragma once

#include <QAbstractSocket>

class QTcpSocket;
class QJsonObject;

class Connection : public QObject
{
  Q_OBJECT
public:
  Connection(const QString &clientId, QTcpSocket *socket, QObject *parent = nullptr);
  ~Connection();

  const QString& ClientId() const noexcept{ return cid_; }
  void SendJson(const QJsonObject &obj);
  void DisconnectSocket();

private:
  QTcpSocket *socket_ = nullptr;
  QString cid_;

private slots:
  void OnReadyRead();
  void OnSocketError(QAbstractSocket::SocketError);

signals:
  void JsonObject(const QJsonObject&);
  void Disconnected();
  void ErrorOccurred(const QString &, const QString &);
};
