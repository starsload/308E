﻿#include "AirConditionHost.h"
#include "UseDatabase.h"
#include "inovice.h"
#include "report.h"

AirConditionHost::AirConditionHost(QObject *parent):
    QObject(parent)
{
}

AirConditionHost::~AirConditionHost() {
    delete server;
}

void AirConditionHost::PowerOn() {
    server = new QTcpServer();

	tmpDB = QSqlDatabase::addDatabase("QODBC");
	db = &tmpDB;
	qDebug()<<"ODBC driver is valid? "<<db->isValid();
	QString dsn = QString::fromLocal8Bit("ACCMS_R");     //数据源名称
	db->setHostName("112.74.57.177");
	db->setDatabaseName(dsn);                            //设置数据源名称
	db->setUserName("sa");                               //登录用户
	db->setPassword("308eWORK");                         //密码
	if (!db->open()){
		qDebug()<<"数据库打开失败";
		qDebug()<<db->lastError().text();
	}

	QDateTime dateTime(QDateTime::currentDateTime());
	Date = dateTime.toString("yyyy-MM-dd");
	qDebug() << Date;
	InsertACCchart(Date,*db);


    CreateWaitList();
    CreateServiceList();
	CreateMonitor();
	CreatChartController();
	CreateSchduleController();
	//qDebug()<<"请输入ManagerClient的端口：";
    QTextStream input(stdin);
	quint16 port = 6666;
	//input >> port;
    connect(server, SIGNAL(newConnection()),
            this, SLOT(managerConnectHandle()));
    if(!server->listen(QHostAddress::Any, port))
    {
        qDebug()<<"服务器监听失败";
        return;
    }
    else
    {
        qDebug()<<"开始等待管理员";
    }
}

void AirConditionHost::managerConnectHandle() {
    QTcpSocket *managerSocket = server->nextPendingConnection();
    if(!managerSocket->isOpen())
        qDebug()<<"连接建立失败";
    else
        qDebug()<<"管理员已连接";
    chartConstroller->setSocket(managerSocket);
    disconnect(server, SIGNAL(newConnection()),
            this, SLOT(managerConnectHandle()));
    server->disconnect();
    server->close();
}

returnRequestOn AirConditionHost::CtreatClient(int Room_Id, double realTemp){
    AirConditionClient *client;
    returnRequestOn r;

    if(this->waitList->FindACC(Room_Id)){//分控机已经存在
        client = this->waitList->FindACC(Room_Id);
        client->Initialize(Room_Id, mode, defaultTargetTemp, realTemp, defaultFeeRate, defaultFanSpeed, *db);
        waitList->PushACC(client);

        r.RoomId = Room_Id;
        r.mode = mode;
        r.curTemp = realTemp;
        r.curFanSpeed = defaultFanSpeed;
        r.totalFee = QueryTotalFee(this->Date,this->Date,Room_Id,*db);
        r.targetTemp = defaultTargetTemp;
    }
    else {//分控机不存在
        client = new AirConditionClient();
        client->Initialize(Room_Id, mode, defaultTargetTemp, realTemp, defaultFeeRate, defaultFanSpeed, *db);
        // 与主机建立关联
        client->setAirConditionHostRelation(this);
        //放入等待队列
        waitList->PushACC(client);

        r.RoomId = Room_Id;
        r.mode = mode;
        r.curTemp = realTemp;
        r.curFanSpeed = defaultFanSpeed;

        r.totalFee = QueryTotalFee(this->Date,this->Date,Room_Id,*db);
        r.targetTemp = defaultTargetTemp;
    }
    return r;
}

void AirConditionHost::setPara(double defaultTargetTemp, double maxTargetTemp, double minTargetTemp,
double highFeeRate, double middleFeeRate, double lowFeeRate, int mode, int speed){
    this->defaultTargetTemp = defaultTargetTemp;
    this->maxTargetTemp = maxTargetTemp;
    this->minTargetTemp = minTargetTemp;
    this->highFeeRate = highFeeRate;
    this->middleFeeRate = middleFeeRate;
    this->lowFeeRate = lowFeeRate;
    this->mode = mode;
    this->defaultFanSpeed = speed;
    switch (speed) {
    case 0:
        this->defaultFeeRate = lowFeeRate;
        break;
    case 1:
        this->defaultFeeRate = middleFeeRate;
        break;
    case 2:
        this->defaultFeeRate = highFeeRate;
        break;
    }
}

void AirConditionHost::startUp() {
	system("cls");
	//qDebug()<<"请输入GuestClientClient的端口：";
    QTextStream input(stdin);
	quint16 port = 6666;
	//input >> port;
    connect(server, SIGNAL(newConnection()),
            this, SLOT(guestConnectHndle()));
    if(!server->listen(QHostAddress::Any, port))
    {
        qDebug()<<"服务器监听失败";
        return;
    }
    else{
        qDebug()<<"开始等待顾客";
    }
}

void AirConditionHost::guestConnectHndle(){
    qDebug()<<"有顾客客户端连接";
    QTcpSocket *socket = server->nextPendingConnection();
    scheduleController->addGuestSocket(socket);
}

void AirConditionHost::CreatChartController() {
    chartConstroller = new ChartController(this);
	chartConstroller->setAirConditionHostRelation(this);
	chartConstroller->setMonitorRelation(monitor);
}

void AirConditionHost::CreateMonitor(){
    monitor = new Monitor();
	monitor->setServiceListRelation(serviceList);
	monitor->setWaitListRelation(waitList);
}

void AirConditionHost::CreateSchduleController(){
    scheduleController = new ScheduleController(this);
    scheduleController->setAirConditionHost(this);
	scheduleController->setMonitorRelation(monitor);
}

void AirConditionHost::CreateWaitList(){
    waitList = new WaitList();
	waitList->Initial();
}

void AirConditionHost::CreateServiceList(){
    serviceList = new ServiceList();
	serviceList->Initial();
}

int AirConditionHost::ChangeTargetTemp(int RoomID,float Temp)//设置温度 先在队列里面找 再去设置
{
    AirConditionClient* mclient;
    if (mclient = waitList->FindACC(RoomID)) {
        mclient->SetTargetTemp(Temp);
    }
	else if (mclient = serviceList->FindACC(RoomID)) {
        mclient->SetTargetTemp(Temp);
    }
    UpdateChangeTempTime(RoomID,this->Date,*db);//db操作
}

int AirConditionHost:: ChangeFanSpeed(int RoomID,float Speed)//改变风速
{
	qDebug()<<"=======================================================";
	qDebug()<<QString("%1号房间改变风速，触发调度").arg(RoomID);
	qDebug()<<QString("改变后的风速为：%1").arg(Speed);
	waitList->debug();
	serviceList->debug();
	qDebug()<<"=======================================================";

    AirConditionClient* mclient;
    AirConditionClient* mVictimclient;
    AirConditionClient* mFrontclient;
    if (mclient = waitList->FindACC(RoomID))
    {
		if(mclient->GetState() == AirConditionClient::STATE_SLEEP)//休眠
            mclient->SetSpeed(Speed);
        else {
			mclient->SetSpeed(Speed);
			if (!serviceList->isEmpty() && mclient->GetPriority() > serviceList->GetMinPriority())//C:
            {
                mVictimclient = serviceList->GetAndPopVictim();//返回一个拷贝对象
				serviceList->PopACC(mVictimclient->GetRoomId());
				waitList->PushACC(mVictimclient);
                mVictimclient->StopRunning();
				scheduleController->SendIdleMsg(mVictimclient->GetRoomId());

                mclient=waitList->PopACC(mclient->GetRoomId());//返回一个拷贝对象
                serviceList->PushACC(mclient);

                mclient->SetSpeed(Speed);
                mclient->StartRunning();
				mclient->DestributeRunTime();
				scheduleController->SendWorkMsg(mclient->GetRoomId());

                UpdateSwitchOnOffTime(mclient->GetRoomId(),this->Date,*db);//db操作 开机
                UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度
            }
        }
    }
	else if (mclient = serviceList->FindACC(RoomID))
    {
		if(waitList->ReadyNum() == 0)//E:
			mclient->SetSpeed(Speed);
		else {
			mclient->SetSpeed(Speed);
			if(mclient->GetPriority() < waitList->GetMaxPriority()) {//B:

				mclient = serviceList->PopACC(mclient->GetRoomId());//返回值是类的拷贝
				waitList->PushACC(mclient);
				mclient->SetSpeed(Speed);
				mclient->StopRunning();
				scheduleController->SendIdleMsg(mclient->GetRoomId());

				mFrontclient = waitList->GetAndPopFrontACC();
				serviceList->PushACC(mFrontclient);
				mFrontclient->StartRunning();
				mFrontclient->DestributeRunTime();
				scheduleController->SendWorkMsg(mFrontclient->GetRoomId());

				UpdateSwitchOnOffTime(mFrontclient->GetRoomId(),this->Date,*db);//db操作 开机


				InsertUseData(mclient->GetRoomId(),mclient->Getget_server_time(),
							  mclient->Getstop_server_time(),mclient->GetTargetTemp(),
							  mclient->GetFanSpeed(),mclient->GetFeeRate(),
							  mclient->GetDuration(),mclient->GetFee(),*db);
				UpdateServiceTime(mclient->GetRoomId(),mclient->GetDuration(),this->Date,*db);
				UpdateTotalFee(mclient->GetRoomId(),mclient->GetFee(),this->Date,*db);
				UpdateDetailRecordNum(mclient->GetRoomId(),this->Date,*db);//一次详单 四件套

				UpdateChangeScheduleTime(mFrontclient->GetRoomId(),this->Date,*db);//db操作 发生调度
				UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度
			}
		}
    }
    UpdateChangeFanSpeedTime(RoomID,this->Date,*db);//db操作
}

//void AirConditionHost::ReachTargetTemperature(int RoomID)//达到目标后提出服务队列到等待队列 并给调度controller发消息
//{
//    AirConditionClient* mFrontclient;
//    AirConditionClient* mclient= serviceList->PopACC(RoomID);
//    mclient->StopRunning();
//    if(waitList->ReadyNum() == 0)
//		waitList->PushACC(mclient);
//    else {
//        waitList->PushACC(mclient);
//        mclient->StopRunning();
//        mFrontclient = waitList->GetAndPopFrontACC();
//        serviceList->PushACC(mFrontclient);
//        mFrontclient->StartRunning();
//        UpdateSwitchOnOffTime(mFrontclient->GetRoomId(),this->Date,*db);//db操作 开机
//    }
//	InsertUseData(mclient->GetRoomId(),mclient->Getget_server_time(),mclient->Getstop_server_time(),
//				  mclient->GetTargetTemp(),mclient->GetFanSpeed(),mclient->GetFeeRate(),
//				  mclient->GetDuration(),mclient->GetFee(),*db);
//    UpdateServiceTime(mclient->GetRoomId(),mclient->GetDuration(),this->Date,*db);
//    UpdateTotalFee(mclient->GetRoomId(),mclient->GetFee(),this->Date,*db);
//    UpdateDetailRecordNum(mclient->GetRoomId(),this->Date,*db);//一次详单 四件套

//    UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度

//}

//void AirConditionHost::TimeOff(int RoomId,float FeeRate) {
//    AirConditionClient* mclient = serviceList->FindACC(RoomId);
//    AirConditionClient* mFrontclient;
//    if (FeeRate > waitList->GetMaxPriority())
//    {
//        mclient->DestributeRunTime();
//    }
//    else {
//        mclient = serviceList->PopACC(RoomId);
//        mFrontclient = waitList->GetAndPopFrontACC();

//        waitList->PushACC(mclient);2
//        mclient->StopRunning();
//        serviceList->PushACC(mFrontclient);
//        mFrontclient->StartRunning();
//        mFrontclient->DestributeRunTime();

//        UpdateSwitchOnOffTime(mFrontclient->GetRoomId(),this->Date,*db);//db操作 开机

//		InsertUseData(mclient->GetRoomId(),mclient->Getget_server_time(),mclient->Getstop_server_time(),
//					  mclient->GetTargetTemp(),mclient->GetFanSpeed(),mclient->GetFeeRate(),
//					  mclient->GetDuration(),mclient->GetFee(),*db);
//		UpdateServiceTime(mclient->GetRoomId(),mclient->GetDuration(),this->Date,*db);
//        UpdateTotalFee(mclient->GetRoomId(),mclient->GetFee(),this->Date,*db);
//        UpdateDetailRecordNum(mclient->GetRoomId(),this->Date,*db);//一次详单 四件套


//        UpdateChangeScheduleTime(mFrontclient->GetRoomId(),this->Date,*db);//db操作 发生调度
//        UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度
//    }

//}

bool AirConditionHost::RequestService(int RoomId, float PreTemp) {
    AirConditionClient* mclient = waitList->FindACC(RoomId);    //查找房间号对应的client
    mclient->SetPreTemp(PreTemp);
    AirConditionClient* mVictimclient;  //被牺牲的client
	bool flag;
    if (!serviceList->isFull()) //服务队列未满
    {
        mclient = waitList->PopACC(RoomId);     //从等待队列弹出client
        serviceList->PushACC(mclient);  //服务队列加入mclient
        mclient->StartRunning();    //mclient开始运行
        mclient->DestributeRunTime();   //给mclient分配时间片
        UpdateSwitchOnOffTime(mclient->GetRoomId(),this->Date,*db);//db操作 开机
        UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度

		flag = true;
    }
    else if(mclient->GetPriority()> serviceList->GetMinPriority()) {    //mclient的优先级大于服务队列中的最小优先级
        mVictimclient = serviceList->GetAndPopVictim();//找到牺牲者
        waitList->PushACC(mVictimclient);   //将牺牲者加入等待队列
        mVictimclient->StopRunning();   //牺牲者停止服务
        mclient = waitList->PopACC(mclient->GetRoomId());   //从等待队列移出mclient
        serviceList->PushACC(mclient);  //mclient加入服务队列
        mclient->StartRunning();    //mclient开始服务
        mclient->DestributeRunTime();   //mclient分配时间片

        UpdateSwitchOnOffTime(mclient->GetRoomId(),this->Date,*db);//db操作 开机

        InsertUseData(mclient->GetRoomId(),mclient->Getget_server_time(),mclient->Getstop_server_time(),
                      mclient->GetTargetTemp(),mclient->GetFanSpeed(),mclient->GetFeeRate(),
                      mclient->GetDuration(),mclient->GetFee(),*db);
        UpdateServiceTime(mclient->GetRoomId(),mclient->GetDuration(),this->Date,*db);
        UpdateTotalFee(mclient->GetRoomId(),mclient->GetFee(),this->Date,*db);
        UpdateDetailRecordNum(mclient->GetRoomId(),this->Date,*db);//记录详单

        UpdateChangeScheduleTime(mclient->GetRoomId(),this->Date,*db);//db操作 发生调度
        UpdateChangeScheduleTime(mVictimclient->GetRoomId(),this->Date,*db);//db操作 发生调度

		flag = true;
    }
    else {
		//waitList->PushACC(mclient);  //将请求客户端加入等待队列
		flag = false;
    }

	qDebug()<<"=======================================================";
	qDebug()<<QString("%1号房间提出RequestService请求，触发调度").arg(RoomId);
	qDebug()<<QString("服务队列中有%1个分控机").arg(serviceList->getSize());
	qDebug()<<"=======================================================";

	return flag;
}

Inovice AirConditionHost::CreateRDR(int RoomID, QString data_in, QString data_out)//请求数据库 返回详单对象
{
    Inovice detail(RoomID,*db);
    detail.detail = QueryDataInUseData(RoomID,*db);
    return detail;
}

Report AirConditionHost::CreateReport(vector<int> listRoomId,int typeReport,QString date)//请求数据库 返回报表对象
{
    Report mReport=Report(date,*db);
    mReport.report = QueryDataInACCchart(date,*db);

    return mReport;
}

float AirConditionHost::CreateInvoice(int RoomID, QString data_in, QString data_out)//请求数据库 返回总花费
{
    return QueryTotalFee(data_in,data_out,RoomID,*db);
}

void AirConditionHost::TurnOff(int RoomId)//关闭指定分控机
{
	AirConditionClient* client = NULL;
	AirConditionClient* temp = NULL;
	float tempFee,tempDuration;
	client = waitList->FindACC(RoomId);
	if(client!=NULL){//在等待队列
		client->SetSleep();
		tempFee = client->GetFee();
		tempDuration = client->GetDuration();

	}
	else {//在服务队列
		//将分控机从服务队列移出并保存参数
		client = serviceList->FindACC(RoomId);
		if(client != NULL){
			client->SetSleep();
			tempFee = client->GetFee();
			tempDuration = client->GetDuration();
			serviceList->PopACC(RoomId);

			//将等待队列中优先级最高的取出
			if(waitList->ReadyNum() > 0){
				temp=waitList->GetAndPopFrontACC();
				serviceList->PushACC(temp);
				temp->StartRunning();
				temp->DestributeRunTime();
				InsertUseData(temp->GetRoomId(),temp->Getget_server_time(),temp->Getstop_server_time(),temp->GetTargetTemp(),
								 temp->GetFanSpeed(),temp->GetFeeRate(),temp->GetDuration(),temp->GetFee(),*db);
			}
		}
		else //两个队列都不在，说明之前已经关机过，可能是断连的情况
			return;
	}

	//将状态插入到数据库
	InsertUseData(RoomId,client->Getget_server_time(),client->Getstop_server_time(),
				  client->GetTargetTemp(),client->GetFanSpeed(),client->GetFeeRate(),tempDuration,tempFee,*db);

	UpdateTotalFee(RoomId, client->GetFee(), Date, *db);

	UpdateChangeScheduleTime(RoomId,this->Date,*db);

	delete client;//删掉分控机
}

void AirConditionHost:: TimeOff(int RoomId)//时间片到的调度
{
	qDebug()<<"=======================================================";
	qDebug()<<QString("%1号房间时间片到，触发时间片调度").arg(RoomId);
	waitList->debug();
	serviceList->debug();
	qDebug()<<"=======================================================";

	AirConditionClient *client,*temp ;
	if(waitList->ReadyNum() > 0) {//等待队列不为空
		float max = this->waitList->GetMaxPriority();//找到等待队列中最高优先级的分控机的优先级

		if(max<this->serviceList->FindACC(RoomId)->Getpriority())//时间片到达的分控机优先级更高
		{
			//重新分配时间片
			client = this->serviceList->FindACC(RoomId);
			InsertUseData(client->GetRoomId(),client->Getget_server_time(),client->Getstop_server_time(),client->GetTargetTemp(),
						  client->GetFanSpeed(),client->GetFeeRate(),client->GetDuration(),client->GetFee(),*db);
			client->DestributeRunTime();
		}
		else//等待队列中有优先级更高的分控机存在
		{
		   //将服务队列中的分控机取出
		   client = this->serviceList->FindACC(RoomId);
		   this->serviceList->PopACC(RoomId);
		   this->waitList->PushACC(client);
		   client->StopRunning();
		   scheduleController->SendIdleMsg(client->GetRoomId());

		   InsertUseData(client->GetRoomId(),client->Getget_server_time(),client->Getstop_server_time(),client->GetTargetTemp(),
						 client->GetFanSpeed(),client->GetFeeRate(),client->GetDuration(),client->GetFee(),*db);


		   //将等待队列中的分控机放入服务队列
		   temp = this->waitList->GetAndPopFrontACC();
		   this->serviceList->PushACC(temp);
		   //改变分控机状态并分配时间片
		   temp->StartRunning();
		   temp->DestributeRunTime();
		   scheduleController->SendWorkMsg(temp->GetRoomId());

		   InsertUseData(temp->GetRoomId(),temp->Getget_server_time(),temp->Getstop_server_time(),temp->GetTargetTemp(),
						 temp->GetFanSpeed(),temp->GetFeeRate(),temp->GetDuration(),temp->GetFee(),*db);
		}
	}
	else {//等待队列为空
		//重新分配时间片
		client = this->serviceList->FindACC(RoomId);
		InsertUseData(client->GetRoomId(),client->Getget_server_time(),client->Getstop_server_time(),client->GetTargetTemp(),
					  client->GetFanSpeed(),client->GetFeeRate(),client->GetDuration(),client->GetFee(),*db);
		client->DestributeRunTime();
	}
}

void AirConditionHost::RearchTargetTemp(int RoomId)//到达目标温度调度
{
	qDebug()<<"=======================================================";
	qDebug()<<QString("%1号房间达到目标温度，触发调度").arg(RoomId);

	//找出分控机对象
	AirConditionClient *client;
	AirConditionClient *temp;

	//移出服务队列
	client = serviceList->PopACC(RoomId);

	//设置分控机对象参数
	client->StopRunning();
	client->SetSleep();//状态为休眠
	scheduleController->SendStopMsg(RoomId, client->GetFee(), client->GetTotalFee(), client->GetPreTemp());

	//调度部分
	if(waitList->ReadyNum() > 0) // 等待队列不为空发生调度
	{
		//将等待队列最高优先级分控机取出放入服务队列
		temp = waitList->GetAndPopFrontACC();
		serviceList->PushACC(temp);

		//改变分控机状态并分配时间片
		temp->StartRunning();
		temp->DestributeRunTime();
		scheduleController->SendWorkMsg(temp->GetRoomId());
	}

	//将完成服务的分控机移入等待队列
	waitList->PushACC(client);
//	waitList->ReadyNum();

	//数据库操作
	InsertUseData(client->GetRoomId(),client->Getget_server_time(),client->Getstop_server_time(),client->GetTargetTemp(),
				  client->GetFanSpeed(),client->GetFeeRate(),client->GetDuration(),client->GetFee(),*db);


}
