#pragma once

#include <QObject>
#include <QHash>

class QTcpServer;
class Connection;

class ConnectionMan : public QObject
{
  Q_OBJECT
public:
  explicit ConnectionMan(quint16 port = 12345, QObject *parent = nullptr);
  void SetCPUwarn(qint32 v) noexcept {cpuWarn_ = v;}
  ~ConnectionMan();

private:
  QTcpServer *tcpServer_ = nullptr;
  quint16 port_ = 0;
  QHash<QString, Connection*> clients_;
  quint32 nextClientId_ = 0;
  qint32 cpuWarn_ = 0;

public slots:
  void StartServer();
  void StopServer();
  void StartClients();
  void StopClients();

private slots:
  void HandleNewConnection();
  void HandleClientReadyRead(const QJsonObject& obj);
  void HandleClientDisconnected();
  void HandleClientError(const QString &clientId, const QString &errmsg);

signals:
  void ClientConnected(const QString &clientId, const QString &ip, quint16 port);
  void ClientDisconnected(const QString &clientId);
  void DataReceived(const QString &clientId, const QJsonObject &obj);
  void LogMessage(const QString &msg);
};
