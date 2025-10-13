#include "connectionman.h"
#include "connection.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDateTime>

ConnectionMan::ConnectionMan(quint16 port, QObject *parent)
    : QObject(parent), tcpServer_(nullptr), port_(port), nextClientId_(1)
{
}

ConnectionMan::~ConnectionMan()
{
  StopServer();
}

void ConnectionMan::StartServer()
{
  if (tcpServer_) {
    emit LogMessage("Server already started.");
    return;
  }
  tcpServer_ = new QTcpServer();
  if (!tcpServer_->listen(QHostAddress::Any, port_)) {
    emit LogMessage(QString("Failed to start server: %1").arg(tcpServer_->errorString()));
    delete tcpServer_;
    tcpServer_ = nullptr;
    return;
  }
  connect(tcpServer_, &QTcpServer::newConnection, this, &ConnectionMan::HandleNewConnection);
  emit LogMessage(QString("Server listening on port %1").arg(port_));
}

void ConnectionMan::StopServer()
{
  if (!tcpServer_)
    return;
  tcpServer_->close();
  delete tcpServer_;
  tcpServer_ = nullptr;
  for (auto c : std::as_const(clients_)) {
    c->DisconnectSocket();
    c->deleteLater();
  }
  clients_.clear();
  emit LogMessage("Server stopped.");
}

void ConnectionMan::HandleNewConnection()
{
  if(!tcpServer_)
    return;

  while (tcpServer_->hasPendingConnections()) {
    auto sock = tcpServer_->nextPendingConnection();
    auto cid = QString("Client_%1").arg(nextClientId_++);
    auto conn = new Connection(cid, sock, this);
    clients_.insert(cid, conn);

    connect(conn, &Connection::Disconnected, this, &ConnectionMan::HandleClientDisconnected);
    connect(conn, &Connection::JsonObject, this, &ConnectionMan::HandleClientReadyRead);
    connect(conn, &Connection::ErrorOccurred, this, [this](const QString &id, const QString &err){
      HandleClientError(id, err);
    });

    QJsonObject confirm;
    confirm["type"] = "ConnectAck";
    confirm["clientId"] = cid;
    confirm["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    conn->SendJson(confirm);

    auto ip = sock->peerAddress().toString();
    auto port = sock->peerPort();

    emit ClientConnected(cid, ip, port);
    emit LogMessage(QString("New connection %1 from %2:%3").arg(cid,ip,port));
  }
}

void ConnectionMan::HandleClientReadyRead(const QJsonObject &obj)
{
  auto conn = qobject_cast<Connection*>(sender());
  if(conn)
    emit DataReceived(conn->ClientId(), obj);
}

void ConnectionMan::HandleClientDisconnected()
{
  auto conn = qobject_cast<Connection*>(sender());
  if(conn){
    auto id = conn->ClientId();
    clients_.remove(id);
    emit ClientDisconnected(id);
    emit LogMessage(QString("Client %1 disconnected").arg(id));
    conn->deleteLater();
  }
}

void ConnectionMan::HandleClientError(const QString &clientId, const QString &errmsg)
{
  emit LogMessage(QString("Client %1 error: %2").arg(clientId,errmsg));
}

void ConnectionMan::StartClients()
{
  QJsonObject cmd;
  cmd["type"] = "Command";
  cmd["command"] = "start";
  cmd["cpuWarn"] = cpuWarn_;

  for(auto it = clients_.begin(); it != clients_.end(); ++it)
    it.value()->SendJson(cmd);
}

void ConnectionMan::StopClients()
{
  QJsonObject cmd;
  cmd["type"] = "Command";
  cmd["command"] = "stop";
  for(auto it = clients_.begin(); it != clients_.end(); ++it)
    it.value()->SendJson(cmd);
}
