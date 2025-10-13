#pragma once

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
  // обработчики для стандартных сигналов от qtcpsocket
  void OnConnected();
  void OnDisconnected();
  void OnReadyRead();
  void OnSocketError(QAbstractSocket::SocketError);

  // переподключение к серверу
  void Reconnect();
  // генерация сообщения для сервера
  void SendDataToServer();

private:
  QJsonObject LogObject(const QString& saverity, const QString& msg);
  // обработчик сообщений от сервера
  void ProcessJSON(const QJsonObject &obj);
  // отправка сгенерированного сообщения в виде QJsonObject
  void SendJson(const QJsonObject &obj);
  // генерация рандомных значений
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
