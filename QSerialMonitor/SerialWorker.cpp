#include "SerialWorker.h"

SerialWorker::SerialWorker(QObject* parent)
	:QObject(parent)
{
	m_serialPort = new QSerialPort(this);
	connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
	connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
}

SerialWorker::~SerialWorker()
{
	closePort();
}

void SerialWorker::openPort(const QString& portName, qint32 baudRate,
	QSerialPort::DataBits dataButs,
	QSerialPort::Parity parity,
	QSerialPort::StopBits stopBits)
{
	m_serialPort->setPortName(portName);
	m_serialPort->setBaudRate(baudRate);
	m_serialPort->setDataBits(dataButs);
	m_serialPort->setParity(parity);
	m_serialPort->setStopBits(stopBits);
	m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

	if (!m_serialPort->open(QIODevice::ReadWrite))
	{
		emit errorOccurred(QStringLiteral("串口打开失败：%1").arg(m_serialPort->errorString()));
		return;
	}

	m_readBuffer.clear();
	emit portOpened();
}

void SerialWorker::closePort()
{
	if (m_serialPort->isOpen())
	{
		m_serialPort->close();
	}

	m_readBuffer.clear();
	emit portClosed();
}

void SerialWorker::writeText(const QString& text)
{
	if (!m_serialPort->isOpen())
	{
		emit errorOccurred(QStringLiteral("串口未打开，无法发送数据"));
		return;
	}
	QByteArray data = text.toUtf8();
	if(!data.endsWith('\n'))
	{
		data.append('\n');
	}
	m_serialPort->write(data);
}

void SerialWorker::handleReadyRead()
{
	QByteArray receiveData = m_serialPort->readAll();
	// 发送原始数据
	emit rawReceived(receiveData);

	// 将接收到的数据追加到缓冲区
	m_readBuffer.append(receiveData);

	// 以换行符为分隔符，提取完整的行
	int endIndex = m_readBuffer.indexOf('\n');
	while (endIndex >= 0)
	{
		// 提取完整行，不包含换行符
		QByteArray line = m_readBuffer.left(endIndex);
		// 移除缓冲区中的已处理数据，包括换行符
		m_readBuffer.remove(0, endIndex + 1);
		// 去除行首尾的空白字符
		line = line.trimmed(); 
		// 发射信号，通知接收到完整行
		QString text = QString::fromUtf8(line);
		emit lineReceived(text);
		// 查找下一个换行符，循环处理剩余数据
		endIndex = m_readBuffer.indexOf('\n');
	}
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
	if (error == QSerialPort::ResourceError)
	{
		emit errorOccurred(QStringLiteral("串口发生资源错误：%1").arg(m_serialPort->errorString()));
		closePort();
	}
}