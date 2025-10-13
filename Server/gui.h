#pragma once

#include <QMainWindow>

class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class QTableWidget;
class QTabWidget;
class QTextBrowser;
class ConnectionMan;
class QSpinBox;
class QThread;

class CentralWidget : public QWidget
{
  Q_OBJECT
public:
  explicit CentralWidget(QWidget *parent = nullptr);
  void InitializeServer();
  ~CentralWidget();

private:
  // лэйаут управления
  QHBoxLayout *layControl_ = nullptr;
  QPushButton *sStart_ = nullptr;
  QPushButton *sStop_ = nullptr;
  QPushButton *cStart_ = nullptr;
  QPushButton *cStop_ = nullptr;
  QSpinBox *cpuWarn_ = nullptr;

         // главный лэйаут содержащий осноынве графические компоненты
  QVBoxLayout *lay_ = nullptr;
  QTabWidget  *tab_ = nullptr;

  QTableWidget *clients_ = nullptr;
  QTableWidget *messages_ = nullptr;
  QTextBrowser *log_ = nullptr;

         // бизнес-логика в отдельном потоке
  ConnectionMan *worker_ = nullptr;
  QThread *workerThread_ = nullptr;

  QMap<QString,int> clientRows_;

  void AddClientRow(const QString &clientId, const QString &ip, const QString &status);
  void RemoveClientRow(const QString &clientId);

private slots:
  void OnStartServer();
  void OnStopServer();
  void OnClientConnected(const QString &clientId, const QString &ip, quint16 port);
  void OnClientDisconnected(const QString &clientId);
  void OnDataReceived(const QString &clientId, const QJsonObject &obj);
  void OnLogMessage(const QString &msg);

  void OnStartClientsClicked();
  void OnStopClientsClicked();
};

////////////////////////////////////////////

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(QWidget * parent = nullptr) : QMainWindow(parent)
  {
    auto cw = new CentralWidget(this);
    cw->InitializeServer();
    setCentralWidget(cw);
  }
};
