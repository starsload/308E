﻿#ifndef AIRCONDITIONHOST_H
#define AIRCONDITIONHOST_H

#include <QTcpServer>
#include <QTime>
#include <ChartController.h>
#include <ScheduleController.h>
#include <Monitor.h>
#include <WaitList.h>
#include <AirConditionClient.h>
#include <QObject>
#include <QVector>
#include <ServiceList.h>
#include <Report.h>
#include <Invoice.h>
#include "Invoice.h"
#include "DetailRecords.h"

class returnRequestOn{
public:
    int RoomId;
    double curTemp;
    double targetTemp;
    int curFanSpeed;
    int mode;
    double totalFee;
};

class returnChangeFanSpeed{
public:
	int FeeRate;
	bool isRunning;
};

class AirConditionHost: public QObject
{
    Q_OBJECT

public:
    explicit AirConditionHost(QObject *parent = nullptr);
    ~AirConditionHost();

    void PowerOn();

    void setPara(double defaultTargetTemp,
                 double maxTargetTemp,
                 double minTargetTemp,
                 double highFeeRate,
                 double middleFeeRate,
                 double lowFeeRate,
                 int mode,
                 int speed);

    void startUp();

    void CreatChartController();

    void CreateMonitor();

    void CreateSchduleController();

    void CreateWaitList();

    void CreateServiceList();

    returnRequestOn CtreatClient(int Room_Id, double realTemp);

	int ChangeTargetTemp(int RoomID,float Temp);//设置温度 先在队列里面找 再去设置

	int ChangeFanSpeed(int RoomID,float Speed);//改变风速

    bool RequestService(int RoomId, float PreTemp);//请求服务

	void TurnOff(int RoomId);

	void setMonitorRelation();

	void TimeOff(int RoomId);//时间片到的调度

	void RearchTargetTemp(int RoomId);//到达目标温度进行的调度,参数为需要处理的分控机房间号

	Invoice MakeAnInvoice(int RoomID);//请求数据库，返回账单

	DetailRecords MakeDR(int RoomID);//请求数据库 返回详单

	Report RequestReport(QString  date);//请求数据库 返回报表

	void RequestPrintReport(Report); //打印报表

private slots:
    void managerConnectHandle();

    void guestConnectHndle();

private:
    QTcpServer *server;
    ChartController *chartConstroller;
    ScheduleController *scheduleController;
    Monitor *monitor;
    WaitList *waitList;
    ServiceList *serviceList;

	QSqlDatabase tmpDB;
	QSqlDatabase *db;
	quint16 port;

    double defaultTargetTemp; //默认目标温度
    double maxTargetTemp; //最高目标温度
    double minTargetTemp; //最低目标温度
    double highFeeRate; //高风速费率
    double middleFeeRate; //中风速费率
    double lowFeeRate; //低风速费率
    double defaultFeeRate; //默认费率
    int defaultFanSpeed; //默认风速：0为低，1为中，2为高
    int mode; //工作模式：0是制冷，1是制热

	QString Date = "2020-6-8";//日期

    QTime current_time;
};

#endif // AIRCONDITIONHOST_H
