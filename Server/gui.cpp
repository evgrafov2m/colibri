#include "gui.h"
#include "connectionman.h"

#include <QJsonObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTableWidget>
#include <QMainWindow>
#include <QSpinBox>
#include <QLabel>
#include <QThread>

static const auto DTFormt = QLatin1String("yyyy-MM-dd hh:mm:ss");

CentralWidget::CentralWidget(QWidget *parent) : QWidget(parent)
{
  setMinimumSize(640,480);
  layControl_ = new QHBoxLayout();
  sStart_ = new QPushButton("Start server",this);
  sStop_ = new QPushButton("Stop server",this);
  sStop_->setEnabled(false);
  cStart_ = new QPushButton("Start clients",this);
  cStart_->setEnabled(false);
  cStop_ = new QPushButton("Stop clients",this);
  cStop_->setEnabled(false);
  cpuWarn_ = new QSpinBox(this);
  cpuWarn_->setRange(1,100);
  cpuWarn_->setValue(50);

  layControl_->addItem(new QSpacerItem(0,0,QSizePolicy::MinimumExpanding));
  layControl_->addWidget(sStart_);
  layControl_->addWidget(sStop_);
  layControl_->addWidget(cStart_);
  layControl_->addWidget(cStop_);
  layControl_->addWidget(new QLabel(QString("Информировать при загрузке процессора более чем"),this));
  layControl_->addWidget(cpuWarn_);
  layControl_->addWidget(new QLabel(QString("%"),this));

  clients_ = new QTableWidget(0,3,this);
  clients_->setHorizontalHeaderLabels(QStringList() << "ID" << "Address" << "Status");
  clients_->setSelectionBehavior(QAbstractItemView::SelectRows);

  messages_ = new QTableWidget(0,5,this);
  messages_->setHorizontalHeaderLabels(QStringList() << "ID" << "Type" << "Content" << "JSON" << "Received");
  messages_->setSelectionBehavior(QAbstractItemView::SelectRows);
  log_ = new QTextBrowser(this);

  lay_ = new QVBoxLayout(this);
  tab_ = new QTabWidget(this);

  tab_->addTab(clients_,"Clients");
  tab_->addTab(messages_,"Messages");
  tab_->addTab(log_,"Log");

  lay_->addLayout(layControl_);
  lay_->addWidget(tab_);
  setLayout(lay_);

  connect(sStart_, &QPushButton::clicked, this, &CentralWidget::OnStartServer);
  connect(sStop_, &QPushButton::clicked, this, &CentralWidget::OnStopServer);
  connect(cStart_, &QPushButton::clicked, this, &CentralWidget::OnStartClientsClicked);
  connect(cStop_, &QPushButton::clicked, this, &CentralWidget::OnStopClientsClicked);
}

void CentralWidget::InitializeServer()
{
  worker_ = new ConnectionMan(12345);
  workerThread_ = new QThread(this);
  worker_->moveToThread(workerThread_);

  connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(this, &CentralWidget::destroyed, workerThread_, &QThread::quit);

  connect(worker_, &ConnectionMan::ClientConnected, this, &CentralWidget::OnClientConnected);
  connect(worker_, &ConnectionMan::ClientDisconnected, this, &CentralWidget::OnClientDisconnected);
  connect(worker_, &ConnectionMan::DataReceived, this, &CentralWidget::OnDataReceived);
  connect(worker_, &ConnectionMan::LogMessage, this, &CentralWidget::OnLogMessage);

  workerThread_->start();
}

CentralWidget::~CentralWidget()
{
  if(worker_)
    QMetaObject::invokeMethod(worker_, "StopServer", Qt::QueuedConnection);

  workerThread_->quit();
  workerThread_->wait();
}

void CentralWidget::OnStartServer()
{
  if(!worker_)
    return;
  QMetaObject::invokeMethod(worker_, "StartServer", Qt::QueuedConnection);
  sStart_->setEnabled(false);
  sStop_->setEnabled(true);
  cStart_->setEnabled(true);
  cStop_->setEnabled(false);
  OnLogMessage("Requested server start.");
}

void CentralWidget::OnStopServer()
{
  if(!worker_)
    return;
  QMetaObject::invokeMethod(worker_, "StopServer", Qt::QueuedConnection);
  sStart_->setEnabled(true);
  sStop_->setEnabled(false);
  OnLogMessage("Requested server stop.");
}

void CentralWidget::OnClientConnected(const QString &clientId, const QString &ip, quint16 port)
{
  auto ipport = QString("%1:%2").arg(ip).arg(port);
  AddClientRow(clientId, ipport, "Connected");
  OnLogMessage(QString("Client connected: %1 (%2)").arg(clientId,ipport));
}

void CentralWidget::OnClientDisconnected(const QString &clientId)
{
  AddClientRow(clientId, "-", "Disconnected");
  OnLogMessage(QString("Client disconnected: %1").arg(clientId));
}

void CentralWidget::OnDataReceived(const QString &clientId, const QJsonObject &obj)
{
  auto type = obj.value("type").toString("Unknown");
  auto raw = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  QString parsed;

  bool warn = false;

  if (type == "NetworkMetrics" || type == "DeviceStatus") {
    QStringList parts;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
      if (it.key() == "type")
        continue;
      auto v = it.value();
      parts << QString("%1=%2").arg(it.key(),v.toVariant().toString());
    }
    parsed = parts.join(", ");
  } else if (type == "Log") {
    auto s = obj.value("severity").toString();
    if(!QString::compare(s,"warn",Qt::CaseInsensitive))
      warn = true;
    parsed = "[" + obj.value("severity").toString() + "] ";
    parsed.append(obj.value("message").toString());

  }

  int row = messages_->rowCount();
  messages_->insertRow(row);
  messages_->setItem(row, 0, new QTableWidgetItem(clientId));
  messages_->setItem(row, 1, new QTableWidgetItem(type));
  messages_->setItem(row, 2, new QTableWidgetItem(parsed));
  messages_->setItem(row, 3, new QTableWidgetItem(raw));
  messages_->setItem(row, 4, new QTableWidgetItem(QDateTime::currentDateTime().toString(DTFormt)));

  if(warn)
    for(int i = 0; i < 5; i++)
      messages_->item(row,i)->setBackground(QColor(255,255,0,40));

  OnLogMessage(QString("Data from %1: %2").arg(clientId,type));
}

void CentralWidget::OnLogMessage(const QString &msg)
{
  log_->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString(DTFormt), msg));
}

void CentralWidget::AddClientRow(const QString &clientId, const QString &ip, const QString &status)
{
  if (clientRows_.contains(clientId)) {
    auto r = clientRows_.value(clientId);
    clients_->setItem(r, 1, new QTableWidgetItem(ip));
    clients_->setItem(r, 2, new QTableWidgetItem(status));
  } else {
    auto row = clients_->rowCount();
    clients_->insertRow(row);
    clients_->setItem(row, 0, new QTableWidgetItem(clientId));
    clients_->setItem(row, 1, new QTableWidgetItem(ip));
    clients_->setItem(row, 2, new QTableWidgetItem(status));
    clientRows_.insert(clientId, row);
  }
}

void CentralWidget::RemoveClientRow(const QString &clientId)
{
  if(!clientRows_.contains(clientId))
    return;
  auto row = clientRows_.take(clientId);
  clients_->removeRow(row);

  QMap<QString,int> tmp;
  for (int r = 0; r < clients_->rowCount(); ++r) {
    auto id = clients_->item(r,0)->text();
    tmp.insert(id, r);
  }
  clientRows_ = tmp;
}

void CentralWidget::OnStartClientsClicked()
{
  if(!worker_)
    return;
  cStart_->setEnabled(false);
  cStop_->setEnabled(true);
  QMetaObject::invokeMethod(worker_,
                            [this]() {
                              worker_->SetCPUwarn(cpuWarn_->value());
                              worker_->StartClients();
                            },Qt::QueuedConnection);
  OnLogMessage("Starting clients ...");
}

void CentralWidget::OnStopClientsClicked()
{
  if(!worker_)
    return;
  cStart_->setEnabled(true);
  cStop_->setEnabled(false);
  QMetaObject::invokeMethod(worker_, "StopClients", Qt::QueuedConnection);
  OnLogMessage("Stopping clients ...");
}
