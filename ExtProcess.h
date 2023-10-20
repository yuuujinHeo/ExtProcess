#ifndef EXTPROCESS_H
#define EXTPROCESS_H

#include <QProcess>
#include <QObject>
#include <QTimer>
#include <QSharedMemory>
#include "GlobalHeader.h"


class ExtProcess : public QObject
{
    Q_OBJECT
public:
    explicit ExtProcess(QObject *parent = nullptr);
    ~ExtProcess();

    enum{
        PROCESS_RETURN_NONE = 0,
        PROCESS_RETURN_ACCEPT,
        PROCESS_RETURN_DONE,
        PROCESS_RETURN_ERROR,
        PROCESS_RETURN_SENDING
    };
    enum{
        PROCESS_CMD_NONE = 0,
        PROCESS_CMD_SET_SYSTEM_VOLUME,
        PROCESS_CMD_GET_SYSTEM_VOLUME,
        PROCESS_CMD_GET_WIFI_LIST,
        PROCESS_CMD_GET_WIFI_LIST_INFO,
        PROCESS_CMD_GET_WIFI_IP,
        PROCESS_CMD_SET_WIFI_IP,
        PROCESS_CMD_CONNECT_WIFI,
        PROCESS_CMD_CHECK_CONNECTION,
        PROCESS_CMD_GIT_PULL,
        PROCESS_CMD_GIT_RESET,
        PROCESS_CMD_GIT_UPDATE

    };

    struct Command{
        uint32_t tick = 0;
        uint32_t cmd = 0;
        uint8_t params[100] = {0,};
        uint8_t params2[100] = {0,};
        uint8_t params3[100] = {0,};
        Command()
        {
        }
        Command(const Command& p)
        {
            tick = p.tick;
            cmd = p.cmd;
            memcpy(params, p.params, 100);
            memcpy(params2, p.params2, 100);
            memcpy(params3, p.params3, 100);
        }
    };


    struct WifiList{
        uint32_t tick = 0;
        uint8_t size = 0;
        uint8_t ssid[10][100] = {0,};
        uint8_t param[10][10] = {0,};
        WifiList(){

        }
        WifiList(const WifiList& p){
            tick = p.tick;
            size = p.size;
            memcpy(ssid, p.ssid, 1000);
            memcpy(param, p.param, 100);

        }
    };

    struct Return{
        uint32_t tick = 0;
        uint32_t result = 0;
        uint32_t command = 0;
        uint8_t params[100] = {0,};
        uint8_t params2[100] = {0,};
        uint8_t params3[100] = {0,};
        Return()
        {
        }
        Return(const Return& p)
        {
            tick = p.tick;
            result = p.result;
            command = p.command;
            memcpy(params, p.params, 100);
            memcpy(params2, p.params2, 100);
            memcpy(params3, p.params3, 100);
        }
    };

    void startProcessAt(QString cmd, QString path);
    void startProcess(QString cmd);
    void set_return(Return ret);

    void connectWifi(QString ssid, QString passwd);
    void setWifiIP(QString ip, QString gateway, QString dns);

    void setSystemVolume(int volume);
    void getSystemVolume();
    void getWifiList();

private slots:
    void onTimer();
    void output();
    void error();
    void timeout();

private:
    int tick = 0;
    Command cur_cmd;

    ST_WIFI_STATE cur_wifi;

    QVector<ST_WIFI> wifi_list;
    QList<Command> cmd_list;
    QProcess *proc;
    QSharedMemory shm_command;
    QSharedMemory shm_return;
    QSharedMemory shm_wifilist;

    Command get_command();
    Return get_return();

    QTimer *timer;
    QTimer *checkTimer;
};

#endif // EXTPROCESS_H
