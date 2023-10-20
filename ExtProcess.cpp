#include "ExtProcess.h"
#include <QDebug>

ExtProcess::ExtProcess(QObject *parent)
    : QObject(parent)
    , shm_command("process_command")
    , shm_return("process_return")
    , shm_wifilist("process_wifilist")
{
    tick = 0;
    checkTimer = new QTimer();
    connect(checkTimer,SIGNAL(timeout()),this,SLOT(timeout()));
    if(!shm_command.isAttached()){
        if (!shm_command.create(sizeof(Command), QSharedMemory::ReadWrite) && shm_command.error() == QSharedMemory::AlreadyExists)
        {
            if(shm_command.attach()){
                qDebug() << "[ExtProcess] SharedMemory is already exist. attach success. ";
//                if(shm_command.isAttached()){
//                    shm_command.lock();
//                    memset(shm_command.data(),0,sizeof(shm_command.data()));
//                    shm_command.unlock();
//                    qDebug() << "[ExtProcess] Clear ShardMemory";
//                }
            }else{
                qDebug() << "[ExtProcess] SharedMemory is already exist. attach failed. ";
            }
        }
        else
        {
            qDebug() << "[ExtProcess] SharedMemory is created. size : "+QString::number(sizeof(Command));
        }
    }
    if(!shm_return.isAttached()){
        if (!shm_return.create(sizeof(Return), QSharedMemory::ReadWrite) && shm_return.error() == QSharedMemory::AlreadyExists)
        {
            if(shm_return.attach()){
                qDebug() << "[ExtProcess] SharedMemory(Return) is already exist. attach success. ";
                if(shm_return.isAttached()){
                    shm_return.lock();
                    memset(shm_return.data(),0,sizeof(shm_return.data()));
                    shm_return.unlock();
                    qDebug() << "[ExtProcess] Clear ShardMemory(Return)";
                }
            }else{
                qDebug() << "[ExtProcess] SharedMemory(Return) is already exist. attach failed. ";
            }
        }
        else
        {
            qDebug() << "[ExtProcess] SharedMemory(Return) is created. size : "+QString::number(sizeof(Return));
        }
    }
    if(!shm_wifilist.isAttached()){
        if (!shm_wifilist.create(sizeof(WifiList), QSharedMemory::ReadWrite) && shm_wifilist.error() == QSharedMemory::AlreadyExists)
        {
            if(shm_wifilist.attach()){
                qDebug() << "[ExtProcess] SharedMemory(WifiList) is already exist. attach success. ";
                if(shm_wifilist.isAttached()){
                    shm_wifilist.lock();
                    memset(shm_wifilist.data(),0,sizeof(shm_wifilist.data()));
                    shm_wifilist.unlock();
                    qDebug() << "[ExtProcess] Clear ShardMemory(WifiList)";
                }
            }else{
                qDebug() << "[ExtProcess] SharedMemory(WifiList) is already exist. attach failed. ";
            }
        }
        else
        {
            qDebug() << "[ExtProcess] SharedMemory(WifiList) is created. size : "+QString::number(sizeof(WifiList));
        }
    }
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(100);
    proc = new QProcess();
}

ExtProcess::~ExtProcess(){
    shm_command.detach();
    shm_return.detach();
}

void ExtProcess::onTimer(){
    static uint32_t prev_tick = 0;

    //Get New Command=========================================================
    ExtProcess::Command _cmd = get_command();
    if(_cmd.tick != prev_tick){
        bool already_in = false;
        for(int i=0; i<cmd_list.size(); i++){
            if(cmd_list[i].cmd == _cmd.cmd){
                already_in = true;
            }
        }
        if(!already_in){
            cmd_list.push_back(_cmd);
        }
        prev_tick = _cmd.tick;
    }

    //Check Command=========================================================
    if(!proc->isOpen()){
        if(cmd_list.size() > 0){
            cur_cmd = cmd_list[0];
            cmd_list.pop_front();

            Return result;
            result.result = PROCESS_RETURN_ACCEPT;
            result.command = cur_cmd.cmd;

            qDebug() << "Set Command : " << cur_cmd.cmd << cmd_list.size();

            if(cur_cmd.cmd == PROCESS_CMD_SET_SYSTEM_VOLUME){
                setSystemVolume(cur_cmd.params[0]);
            }else if(cur_cmd.cmd == PROCESS_CMD_GET_SYSTEM_VOLUME){
                getSystemVolume();
            }else if(cur_cmd.cmd == PROCESS_CMD_GET_WIFI_IP){
                cur_wifi.ip = "";
                cur_wifi.gateway = "";
                cur_wifi.dns = "";
                char temp[100];
                memcpy(temp,cur_cmd.params,sizeof(char)*100);
                QString ssid = QString::fromUtf8(temp);

                if(cur_wifi.ssid == ""){
                    result.result = PROCESS_RETURN_ERROR;
                }else{
                    startProcess("nmcli con show "+cur_wifi.ssid);
                }
                checkTimer->start(1000);
            }else if(cur_cmd.cmd == PROCESS_CMD_GET_WIFI_LIST){
                getWifiList();
            }else if(cur_cmd.cmd == PROCESS_CMD_GET_WIFI_LIST_INFO){
                if(cur_cmd.params[0] < wifi_list.size()){
                    result.result = PROCESS_RETURN_DONE;
                    memcpy(result.params,wifi_list[cur_cmd.params[0]].ssid.toUtf8(),sizeof(char)*100);

                    if(wifi_list[cur_cmd.params[0]].ssid == "mobile_robot_mesh"){
                        qDebug() << "inuse is " << wifi_list[cur_cmd.params[0]].inuse;
                    }
                    result.params2[0] = wifi_list[cur_cmd.params[0]].inuse;
                    result.params2[1] = wifi_list[cur_cmd.params[0]].rate;
                    result.params2[2] = wifi_list[cur_cmd.params[0]].level;
                    result.params2[3] = wifi_list[cur_cmd.params[0]].security;
                    result.params2[4] = wifi_list[cur_cmd.params[0]].discon_count;
                    result.params2[5] = wifi_list[cur_cmd.params[0]].state;
                    result.params2[6] = wifi_list[cur_cmd.params[0]].prev_state;
                }else{
//                    qDebug() << cur_cmd.params[0] << wifi_list.size();
                    result.result = PROCESS_RETURN_ERROR;
                }
            }else if(cur_cmd.cmd == PROCESS_CMD_CONNECT_WIFI){
                char temp[100];
                memcpy(temp,cur_cmd.params,sizeof(char)*100);
                QString ssid = QString::fromUtf8(temp);
                memcpy(temp,cur_cmd.params2,sizeof(char)*100);
                QString passwd = QString::fromUtf8(temp);
                cur_wifi.ssid = ssid;
                connectWifi(ssid,passwd);
            }else if(cur_cmd.cmd == PROCESS_CMD_SET_WIFI_IP){
                char temp[100];
                memcpy(result.params,cur_wifi.ssid.toUtf8(),100);
                memcpy(temp,cur_cmd.params,sizeof(char)*100);
                QString ip = QString::fromUtf8(temp);
                memcpy(temp,cur_cmd.params2,sizeof(char)*100);
                QString gateway = QString::fromUtf8(temp);
                memcpy(temp,cur_cmd.params3,sizeof(char)*100);
                QString dns = QString::fromUtf8(temp);
                setWifiIP(ip, gateway, dns);
            }else if(cur_cmd.cmd == PROCESS_CMD_CHECK_CONNECTION){
//                char temp[100];
//                memcpy(temp,cur_cmd.params,sizeof(char)*100);
//                QString ssid = QString::fromUtf8(temp);
//                cur_wifi.ssid = ssid;
                startProcess("nmcli net con");
//                startProcess("nmcli -f GENERAL.STATE con show "+ssid);
                checkTimer->start(500);
            }else if(cur_cmd.cmd == PROCESS_CMD_GIT_PULL){
                char temp[100];
                memcpy(temp, cur_cmd.params,100);
                startProcessAt("git pull",QString::fromUtf8(temp));
            }else if(cur_cmd.cmd == PROCESS_CMD_GIT_RESET){
                char temp[100];
                memcpy(temp, cur_cmd.params,100);
                startProcessAt("git reset --hard origin/master",QString::fromUtf8(temp));
            }else if(cur_cmd.cmd == PROCESS_CMD_GIT_UPDATE){

            }
            set_return(result);
        }
    }

}

void ExtProcess::getSystemVolume(){
    startProcess("amixer -D pulse sget Master");
}

void ExtProcess::getWifiList(){
    wifi_list.clear();
    startProcess("nmcli device wifi list");
}

void ExtProcess::setSystemVolume(int volume){
    startProcess("amixer -D pulse sset Master "+QString::number(volume)+"%");
}

void ExtProcess::startProcess(QString cmd){
    qDebug() << "startProcess : " << cmd;
    proc = new QProcess();
    proc->start(cmd);
    connect(proc,SIGNAL(readyReadStandardOutput()),this,SLOT(output()));
    connect(proc,SIGNAL(readyReadStandardError()),this,SLOT(error()));
}

void ExtProcess::timeout(){
    Return temp_output;
    temp_output.command = cur_cmd.cmd;
    temp_output.result = PROCESS_RETURN_ERROR;
    memcpy(temp_output.params,cur_cmd.params,sizeof(char)*100);
    set_return(temp_output);
    proc->close();
    checkTimer->stop();
}
void ExtProcess::output(){
    checkTimer->stop();
    QString output = proc->readAllStandardOutput();
//    qDebug() << "Output : " << output;
    Return temp_output;
    temp_output.result = PROCESS_RETURN_DONE;
    temp_output.command = cur_cmd.cmd;
    if(cur_cmd.cmd == PROCESS_CMD_SET_SYSTEM_VOLUME){
        set_return(temp_output);
        proc->close();
    }else if(cur_cmd.cmd == PROCESS_CMD_GET_SYSTEM_VOLUME){
        QString percent = output.split("[")[1].split("]")[0];
        QString cur_volume = percent.split("%")[0];
        temp_output.params[0] = cur_volume.toInt();
        set_return(temp_output);
        proc->close();
    }else if(cur_cmd.cmd == PROCESS_CMD_GET_WIFI_LIST){
        QVector<ST_WIFI> wifi_all;
        QStringList output_lines = output.split("\n");
        for(int i=0; i<output_lines.size(); i++){
            //분해
            QStringList outputs = output_lines[i].split(" ");
            QStringList output_final;
            for(int j=0; j<outputs.size(); j++){
                if(outputs[j] != ""){
                    output_final << outputs[j];
                }
            }

            //IN-USE 체크
            ST_WIFI temp;
            if(output_final.size() > 8){
                if(output_final[0] == "IN-USE"){
                    continue;
                }if(output_final[0] == "*"){
                    if(cur_wifi.ssid != output_final[2]){
                        qDebug() << "wifi ssid auto changed : " << cur_wifi.ssid << output_final[2];
                        cur_wifi.ssid = output_final[2];
                    }
                    temp.inuse = true;
                }else{
                    temp.inuse = false;
                    output_final.push_front(" ");
                }
            }else{
                continue;
            }

            //ssid 체크
            int list_index = -1;
            temp.ssid = output_final[2];
            if(temp.ssid == "--"){
                continue;
            }else{
                for(int i=0; i<wifi_list.size(); i++){
                    if(wifi_list[i].ssid == temp.ssid){
                        list_index = i;
                        if(temp.inuse){
                            wifi_list[i].inuse = temp.inuse;
                        }
                    }
                }
            }

            //새로운거면 push
            if(list_index == -1){
                temp.rate = output_final[5].toInt();
                temp.level = output_final[7].toInt();
                if(output_final[9].left(3) == "WPA"){
                    temp.security = true;
                }else{
                    temp.security = false;
                }
                temp.discon_count = 0;
                wifi_list.push_back(temp);
            }
        }

        Return result;
        result.command = cur_cmd.cmd;
        result.result = PROCESS_RETURN_DONE;
        result.params[0] = wifi_list.size();
        set_return(result);
        proc->close();


        WifiList list;
        list.size = wifi_list.size();
        for(int i=0;i<wifi_list.size(); i++){
            if(i>9)
                break;
            memcpy(list.ssid[i], wifi_list[i].ssid.toUtf8(),sizeof(char)*100);

            list.param[i][0] = wifi_list[i].inuse;
            list.param[i][1] = wifi_list[i].rate;
            list.param[i][2] = wifi_list[i].level;
            list.param[i][3] = wifi_list[i].security;
            list.param[i][4] = wifi_list[i].discon_count;
            list.param[i][5] = wifi_list[i].state;
            list.param[i][6] = wifi_list[i].prev_state;
        }
        shm_wifilist.lock();
        list.tick = ++tick;
        memcpy((char*)shm_wifilist.data(), &list, sizeof(ExtProcess::WifiList));
        shm_wifilist.unlock();

    }else if(cur_cmd.cmd == PROCESS_CMD_GET_WIFI_IP){
        QStringList output_lines = output.split("\n");
        for(int i=0; i<output_lines.size(); i++){
            output_lines[i].replace(" ","");
            QStringList line = output_lines[i].split(":");
//            qDebug() << line;
            if(line.size() > 1){
                if(line[0] == "IP4.ADDRESS[1]"){
                    cur_wifi.ip = line[1].split("/")[0];
                }else if(line[0] == "IP4.GATEWAY"){
                    cur_wifi.gateway = line[1];
                }else if(line[0] == "IP4.DNS[1]"){
                    cur_wifi.dns = line[1];
                }
            }
        }
        if(cur_wifi.ip != "" && cur_wifi.gateway != "" && cur_wifi.dns != ""){
            memcpy(temp_output.params, cur_wifi.ip.toUtf8(),sizeof(char)*100);
            memcpy(temp_output.params2, cur_wifi.gateway.toUtf8(),sizeof(char)*100);
            memcpy(temp_output.params3, cur_wifi.dns.toUtf8(),sizeof(char)*100);
            set_return(temp_output);
            proc->close();
        }
    }else if(cur_cmd.cmd == PROCESS_CMD_CONNECT_WIFI){
        QStringList outputs = output.split(" ");
        for(int i=0; i<outputs.size(); i++){
            if(outputs[i] == "successfully"){
                cur_wifi.connection = true;
                qDebug() << "[SETTING] WIFI CONNECT SUCCESS";
                memcpy(temp_output.params, cur_cmd.params, 100);
                set_return(temp_output);
            }else if(outputs[i] == "failed:"){
                temp_output.result = PROCESS_RETURN_ERROR;
                cur_wifi.connection = false;
                memcpy(temp_output.params,cur_cmd.params,100);
                set_return(temp_output);
            }
        }
        proc->close();
    }else if(cur_cmd.cmd == PROCESS_CMD_SET_WIFI_IP){
        //    wifi_process->execute(exe_str);
        //    wifi_process->waitForStarted();
        //    wifi_process->start("nmcli con up "+probot->wifi_ssd);
        //    wifi_process->waitForStarted();
        memcpy(temp_output.params,cur_cmd.params,100);
        memcpy(temp_output.params2,cur_cmd.params2,100);
        memcpy(temp_output.params3,cur_cmd.params3,100);
        set_return(temp_output);
        proc->close();
    }else if(cur_cmd.cmd == PROCESS_CMD_CHECK_CONNECTION){
        QStringList tt = output.split("\n");
        memcpy(temp_output.params, tt[0].toUtf8(),100);
        set_return(temp_output);
        proc->close();
    }else if(cur_cmd.cmd == PROCESS_CMD_GIT_PULL){

    }

}

void ExtProcess::error(){
    checkTimer->stop();
    QString error = proc->readAllStandardOutput();
    qDebug() << "Error : " << error;
    Return temp;
    temp.result = PROCESS_RETURN_ERROR;
    temp.command = cur_cmd.cmd;
    if(cur_cmd.cmd == PROCESS_CMD_CONNECT_WIFI){
        cur_wifi.connection = false;
        memcpy(temp.params,cur_cmd.params,100);

    }else if(cur_cmd.cmd == PROCESS_CMD_SET_WIFI_IP){
        if(cur_wifi.connection){
            cur_wifi.connection = false;
            startProcess("nmcli con up "+cur_wifi.ssid);
        }else{
            set_return(temp);
        }
    }else if(cur_cmd.cmd == PROCESS_CMD_GIT_PULL){

    }else if(cur_cmd.cmd == PROCESS_CMD_GIT_RESET){

    }
    set_return(temp);
    proc->close();
}

void ExtProcess::set_return(Return cmd){
    shm_return.lock();
    cmd.tick = ++tick;
    memcpy((char*)shm_return.data(), &cmd, sizeof(ExtProcess::Command));

    qDebug() << "Set Return : " << cmd.command << cmd.result << cmd.params[0];

    shm_return.unlock();
}
ExtProcess::Return ExtProcess::get_return(){
    ExtProcess::Return res;
    shm_return.lock();
    memcpy(&res, (char*)shm_return.constData(), sizeof(ExtProcess::Return));
    shm_return.unlock();

    return res;
}

ExtProcess::Command ExtProcess::get_command(){
    ExtProcess::Command res;
    shm_command.lock();
    memcpy(&res, (char*)shm_command.constData(), sizeof(ExtProcess::Command));
    shm_command.unlock();
    return res;
}

void ExtProcess::connectWifi(QString ssid, QString passwd){
    if(passwd == ""){
        startProcess("nmcli --a device wifi connect "+ssid);
    }else{
        startProcess("nmcli --a device wifi connect "+ssid+" password "+passwd);
    }
}

void ExtProcess::setWifiIP(QString ip, QString gateway, QString dns){
    qDebug() << "[SETTING] SET WIFI IP "+cur_wifi.ssid+" -> "+ip+", "+gateway+", "+dns;
    QString exe_str = "nmcli con mod "+cur_wifi.ssid;
    exe_str += " ipv4.address "+ip+"/24";
    exe_str += " ipv4.dns "+dns;
    exe_str += " ipv4.gateway "+gateway;
    exe_str += " ipv4.method manual";
    startProcess(exe_str);
    startProcess("nmcli con up "+cur_wifi.ssid);
    //    wifi_process->execute(exe_str);
    //    wifi_process->waitForStarted();
    //    wifi_process->start("nmcli con up "+probot->wifi_ssd);
    //    wifi_process->waitForStarted();
    //  proc->close();
}
