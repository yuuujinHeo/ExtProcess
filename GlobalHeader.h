#ifndef GLOBALHEADER_H
#define GLOBALHEADER_H

#include <QVector>
#include <QString>
#include "Logger.h"

extern Logger *plog;
typedef struct{
    bool inuse;
    QString ssid;
    int rate;
    int level;
    bool security;
    int discon_count;
    int state;
    int prev_state=0;
}ST_WIFI;

typedef struct{
    bool connection;
    QString ip;
    QString gateway;
    QString dns;
    QString ssid;
    QString passwd;
}ST_WIFI_STATE;

#endif // GLOBALHEADER_H
