#include "client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QCoreApplication>
#include <QTextStream>
#include <QDebug>
#include <QRandomGenerator>
#include <QDataStream>

Client::Client(QObject *parent) : QObject(parent)
{
  static constexpr int rInterval = 1000 * 5; // 5sec интервал переподключения
  recTimer_.setInterval(rInterval);
  sendTimer_.setSingleShot(true);

  appTimer_.start();

  connect(&socket_, &QTcpSocket::connected, this, &Client::OnConnected);
  connect(&socket_, &QTcpSocket::disconnected, this, &Client::OnDisconnected);
  connect(&socket_, &QTcpSocket::readyRead, this, &Client::OnReadyRead);
  connect(&socket_, &QAbstractSocket::errorOccurred,this, &Client::OnSocketError);
  connect(&recTimer_, &QTimer::timeout, this, &Client::Reconnect);
  connect(&sendTimer_, &QTimer::timeout, this, &Client::SendDataToServer);
}

Client::~Client()
{
  socket_.close();
}

void Client::Start(const QString &host, quint16 port)
{
  host_ = host;
  port_ = port;

  Reconnect();
}

void Client::Reconnect()
{
  if(socket_.state() == QAbstractSocket::ConnectedState)
    return;

  qDebug() << QString("Connecting to %1:%2 ...").arg(host_).arg(port_);
  socket_.connectToHost(host_, port_);
}

void Client::OnConnected()
{
  qDebug() << "Connected";
  recTimer_.stop();
}

void Client::OnDisconnected()
{
  qDebug() << "Disconnected";
  started_ = false;
  sendTimer_.stop();
  recTimer_.start();
}

void Client::OnReadyRead()
{
  QDataStream in(&socket_);
  in.setVersion(QDataStream::Qt_6_0);

  forever {
    in.startTransaction();

    QByteArray jsonData;
    in >> jsonData;

    if (!in.commitTransaction())
      break;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(jsonData, &err);

    if (err.error != QJsonParseError::NoError) {
      qWarning() << "JSON error:" << err.errorString();
      continue;
    }

    if (!doc.isObject())
      continue;

    ProcessJSON(doc.object());
  }
}

void Client::OnSocketError(QAbstractSocket::SocketError)
{
  qCritical() << "Socket error:" << socket_.errorString();
  recTimer_.start();
}

void Client::ProcessJSON(const QJsonObject &obj)
{
  auto type = obj.value("type").toString();

  qDebug() << "Msg type:" << type;

  if(type == "ConnectAck") {
    qDebug() << "ID:" << obj.value("clientId").toString();

  } else if(type == "Command") {
    auto cmd = obj.value("command").toString();
    qDebug() << cmd;

    if(cmd == "start") {
      cpuWarn_ = obj.value("cpuWarn").toInt();
      started_ = true;
      sendTimer_.start(RndInt(10, 100));
    } else if (cmd == "stop") {
      started_ = false;
      sendTimer_.stop();
    }
  }
}

void Client::SendJson(const QJsonObject &obj)
{
  if(socket_.state() != QAbstractSocket::ConnectedState)
    return;

  QByteArray data;
  QDataStream out(&data, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_6_0);

  out << QJsonDocument(obj).toJson(QJsonDocument::Compact);
  socket_.write(data);
  socket_.flush();
}

qint32 Client::RndInt(qint32 from, qint32 to) const noexcept
{
  return QRandomGenerator::global()->bounded(from, to);
}

double Client::RndDouble(double from, double to) const noexcept
{
  return from + QRandomGenerator::global()->generateDouble() * (to - from);
}

void Client::SendDataToServer()
{
  auto t = RndInt(0, 3);
  QJsonObject obj;

  switch (t) {
    case 0: {
      obj["type"] = "NetworkMetrics";
      obj["bandwidth"] = QString::number(RndDouble(1.0, 1000.0),'f',2);
      obj["latency"] = QString::number(RndDouble(1.0, 500),'f',2);
      obj["packet_loss"] = QString::number(RndDouble(0.0, 0.05),'f',2);
      break;
    }
    case 1: {
      auto cpuUsage =  RndInt(0, 100);
      obj["type"] = "DeviceStatus";
      obj["uptime"] = QString::number(appTimer_.elapsed());
      obj["cpu_usage"] = cpuUsage;
      obj["memory_usage"] = RndInt(0, 100);

      if(cpuUsage > cpuWarn_)
        SendJson(LogObject("WARN",QString("CPU usage: %1").arg(cpuUsage)));
      break;
    }
    case 2: {
      static qulonglong mc = 0;
      obj = LogObject("INFO",QString("Random log message number %1").arg(QString::number(++mc)));
      break;
    }
    default:;
  }
  if(!obj.isEmpty())
    SendJson(obj);

  sendTimer_.start(RndInt(10, 100));
}

QJsonObject Client::LogObject(const QString &saverity, const QString &msg)
{
  QJsonObject result;
  result["type"] = "Log";
  result["message"] = msg;
  result["severity"] = saverity;
  return result;
}
