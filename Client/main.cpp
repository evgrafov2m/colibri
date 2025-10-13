#include <QCoreApplication>
#include "client.h"

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  Client client;
  client.Start("127.0.0.1", 12345);

  return a.exec();
}
