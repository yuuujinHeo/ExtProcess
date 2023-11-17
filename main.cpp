#include <QCoreApplication>
#include <QSharedMemory>
#include "ExtProcess.h"
#include "Logger.h"

Logger *plog;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ExtProcess *process = new ExtProcess();

    plog = new Logger();
    return a.exec();
}
