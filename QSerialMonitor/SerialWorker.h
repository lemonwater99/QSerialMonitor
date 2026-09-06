#pragma once

#include <QObject>
#include <QSerialPort>

class SerialWorker : public QObject
{
	Q_OBJECT

public:
	explicit SerialWorker(QObject* parent = nullptr);
	~SerialWorker();

public slots:
	void closePort();
	void openPort(const QString& portName, qint32 baudRate,
		QSerialPort::DataBits dataButs,
		QSerialPort::Parity parity,
		QSerialPort::StopBits stopBits);
	void writeText(const QString& text);

private slots:
	void handleReadyRead();
	void handleError(QSerialPort::SerialPortError error);

signals:
	void portOpened();
	void portClosed();
	void lineReceived(const QString& line);
	void rawReceived(const QByteArray& data);
	void errorOccurred(const QString& message);

private:
	QSerialPort* m_serialPort = nullptr;
	QByteArray m_readBuffer;
};