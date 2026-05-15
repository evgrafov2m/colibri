#pragma once

#include <QJsonObject>
#include <QTimer>
#include <QTcpSocket>
#include <QElapsedTimer>

class Client : public QObject
{
  Q_OBJECT

public:
  explicit Client(QObject *parent = nullptr);
  ~Client();
  void Start(const QString &host, quint16 port);

private slots:
  void OnConnected();
  void OnDisconnected();
  void OnReadyRead();
  void OnSocketError(QAbstractSocket::SocketError);
  void Reconnect();
  void SendDataToServer();

private:
  QJsonObject LogObject(const QString &severity, const QString &msg);
  void ProcessJSON(const QJsonObject &obj);
  void SendJson(const QJsonObject &obj);
  inline qint32 RndInt(qint32 from, qint32 to) const noexcept;
  inline double RndDouble(double from, double to) const noexcept;

  QTcpSocket socket_;

  QString host_;
  quint16 port_ = 0;

  QElapsedTimer appTimer_;
  QTimer recTimer_;
  QTimer sendTimer_;
  qint32 cpuWarn_ = 0;

  bool started_ = false;
};
