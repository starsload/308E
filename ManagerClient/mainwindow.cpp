#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	initialPage = new InitialPage(this);
	connect(initialPage, SIGNAL(initialFinish(InitialParameters))
			, this, SLOT(initialHandle(InitialParameters)));
	initialPage->show();
	initialPage->setModal(true);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::initialHandle(InitialParameters para) {
	parameters = para;
	QString address = para.address;
	QString port = para.port;
	qDebug()<<address<<"========"<<port;
	socket = new QTcpSocket(this);
	socket->connectToHost(address, port.toInt());

	connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
	connect(socket, SIGNAL(disconnected()), this, SLOT(offLine()));
	connect(&socketConnectTimer, &QTimer::timeout, this, &MainWindow::failToConnectServer);
	socketConnectTimer.setSingleShot(true);
	socketConnectTimer.start(5 * 1000);
}

//连接建立成功
void MainWindow::onConnected(){
	socketConnectTimer.stop();
	connect(socket, SIGNAL(readyRead()), this, SLOT(newServerMessage()));
	//传到服务器
	QJsonObject ojson;
	using namespace SocketConstants;
	ojson.insert(TYPE, SET_PARA);
	ojson.insert(DEFAULT_TARGET_TEMP, parameters.defaultTargetTemp);
	ojson.insert(DEFAULT_SPEED, parameters.defaultFanSpeed);
	ojson.insert(MAX_TARGET_TEMP, parameters.maxTargetTemp);
	ojson.insert(MIN_TARGET_TEMP, parameters.minTargetTemp);
	ojson.insert(HIGH_FEE_RATE, parameters.highFeeRate);
	ojson.insert(MID_FEE_RATE, parameters.middleFeeRate);
	ojson.insert(LOW_FEE_RATE, parameters.lowFeeRate);
	ojson.insert(WORK_MODE, parameters.mode);
	sendJSON(ojson);
}

//接收服务器消息
void MainWindow::newServerMessage(){
	using namespace SocketConstants;
	while(socket->bytesAvailable() > 0) {
		QByteArray head;
		QByteArray body;
		head = socket->read(HEAD_LEANGTH);
		if(head.size() == 0){
			continue;
		}
//		qDebug()<<"收到服务器的信息为："<<head;
		int length;
		memcpy(&length, head.data(), sizeof (length));
		for(int i = 0; i < length; i++)
			body.append(socket->read(1));
		qDebug()<<"收到服务器的信息为："<<body;
		//对body进行处理
		processPacket(body);
	}
}

//处理收到的数据包
void MainWindow::processPacket(QByteArray body){
	using namespace SocketConstants;
	QJsonParseError e;
	QJsonDocument doc = QJsonDocument::fromJson(body, &e);
	if(e.error != QJsonParseError::NoError) {
		qDebug() << "JSON格式错误";
		return;
	}

	QJsonObject ojson = doc.object();
	int type = ojson.value(TYPE).toInt();
	switch (type) {
	case SET_PARA_OK:
	{
		QJsonObject ojson;
		ojson.insert(TYPE, START_UP);
		sendJSON(ojson);
		break;
	}
	case START_UP_OK:
		qDebug()<<"start up ok!";
		break;
	}
}

//点击“生成账单”
void MainWindow::on_ptn_makeInvoice_clicked()
{
	queryInvoiceInputDialg = new QueryInvoiceInputDialog(this);
	connect(queryInvoiceInputDialg, SIGNAL(queryFinish(int)),
			this, SLOT(queryInputFinish(int)));
	queryInvoiceInputDialg->show();
	queryInvoiceInputDialg->setModal(true);
}

//返回Room_Id之后
void MainWindow::queryInputFinish(int Room_Id)
{


	invoicePage = new InvoicePage(this, Room_Id);
	invoicePage->show();
}

//点击“生成报表”
void MainWindow::on_ptn_makeReport_clicked()
{
	reportPage = new ReportPage(this);
	reportPage->show();
}

void MainWindow::sendJSON(QJsonObject ojson){
	QJsonDocument doc;
	doc.setObject(ojson);
	QByteArray msg = doc.toJson(QJsonDocument::Compact);
	sendPacket(msg);
}

void MainWindow::sendPacket(QByteArray body){
	QByteArray head;
	// 构造头部
	int  length = body.size();
	int len_int = sizeof(length);
	head.resize(len_int);
	memcpy(head.data(), &length, len_int);
	QByteArray packet;
	packet = head + body;
	socket->write(packet, packet.size());
}

//断线检测
void MainWindow::offLine(){
	QString content("与服务器断开连接，请检查您的网络");
	msgBox = new QMessageBox(nullptr);
	msgBox->setWindowTitle(QString("ERROR"));
	msgBox->setText(content);
	msgBox->setModal(true);
	msgBox->addButton(QMessageBox::Yes);
	msgBox->setIcon(QMessageBox::Critical);
	connect(msgBox, &QMessageBox::destroyed, [=](){
		this->destroy();
	});
	connect(msgBox, &QMessageBox::buttonClicked,
			[=](){
		delete msgBox;
	});
	msgBox->show();
}

//与服务器建立连接失败
void MainWindow::failToConnectServer(){
	QString content("无法与服务器建立连接，请检查您的网络");
	msgBox = new QMessageBox(nullptr);
	msgBox->setWindowTitle(QString("ERROR"));
	msgBox->setText(content);
	msgBox->setModal(true);
	msgBox->addButton(QMessageBox::Yes);
	msgBox->setIcon(QMessageBox::Critical);
	connect(msgBox, &QMessageBox::destroyed, [=](){
		this->destroy();
	});
	connect(msgBox, &QMessageBox::buttonClicked,
			[=](){
		delete msgBox;
	});
	msgBox->show();
}
