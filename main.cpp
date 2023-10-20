#include <QCoreApplication>
#include <QSharedMemory>
#include "ExtProcess.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ExtProcess *process = new ExtProcess();
    return a.exec();
}
