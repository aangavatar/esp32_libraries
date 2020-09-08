/*
 * TinyGsmClientBG96.cpp
 *
 *  Created on: 09-Jul-2019
 *      Author: benaka
 */

#include "TinyGsmClientBG96.h"
#include "time.h"

int TinyGsmBG96::GsmClient::connect(const char *host, uint16_t port)
{
	stop();
	TINY_GSM_YIELD()
	;
	rx.clear();
	sock_connected = at->modemConnect(host, port, mux);
	return sock_connected;
}

int TinyGsmBG96::GsmClient::connect(IPAddress ip, uint16_t port, int timeout)
{
	return connect(ip, port);
}

int TinyGsmBG96::GsmClient::connect(const char *host, uint16_t port,
		int timeout)
{
	return connect(host, port);
}

int Cbg96FileSreamer::available()
{
	if ((m_currBufferBottom < m_currBufferTop)
			|| (m_currBufferTop < m_fileSizeInBytes))
	{
		return true;
	}
	else
	{
		return false;
	}
}

size_t Cbg96FileSreamer::write(uint8_t i)
{
	assert(false);
	return 0;
}

int Cbg96FileSreamer::read()
{
	int data = 0;
	if (m_currBufferBottom < m_currBufferTop)
	{
		data = m_modem.stream.read();
		m_currBufferBottom++;
	}
	else if (m_currBufferTop < m_fileSizeInBytes)
	{
		int64_t recvdLen = m_modem.modemReadFile(m_fileHandle, 64); // todo: lenght 64 has to be optimized
		assert(recvdLen > 0);

		m_currBufferTop += recvdLen;

		data = m_modem.stream.read();
		m_currBufferBottom++;
	}

	return data;
}

size_t Cbg96FileSreamer::readBytes(uint8_t *buffer, size_t length)
{
	int64_t recvdLen = m_modem.modemReadFile(m_fileHandle, length); // todo: lenght 64 has to be optimized
	assert(recvdLen > 0); // todo remove
	const int64_t tempLen = m_modem.stream.readBytes(buffer, recvdLen);
	assert(1 == m_modem.waitResponse(3000));
	assert(tempLen == recvdLen);
	return recvdLen;
}

int Cbg96FileSreamer::seek(uint32_t offset)
{

	m_modem.sendAT(
			GF("+QFSEEK=" + String(m_fileHandle) + "," + String(offset)));
	Serial.println(
			GF("+QFSEEK=" + String(m_fileHandle) + "," + String(offset)));

	assert(m_modem.waitResponse(3000) == 1);
	return 0;
}

uint32_t Cbg96FileSreamer::getCurrentPosition()
{

	m_modem.sendAT(GF("+QFPOSITION=" + String(m_fileHandle)));
	Serial.println(GF("+QFPOSITION=" + String(m_fileHandle)));

//	delay(3000);
//	while(m_modem.stream.available())
//	{
//		char ch = m_modem.stream.read();
//		Serial.print(ch);
//		delay(500);
//	}
//
//	while(1);
	assert(m_modem.waitResponse(3000, GF("+QFPOSITION:")) == 1);
	uint32_t position = m_modem.stream.readStringUntil(' ').toInt();
	Serial.println("position:" + String(position));
	assert(m_modem.waitResponse(3000) == 1);

	return position;
}

int Cbg96FileSreamer::peek()
{
	if (m_fileHandle < 0)
	{
		return -1;
		Serial.println("filehandle pointer less then zero");
	}

	const size_t curPos = getCurrentPosition();
	uint8_t result = -1;
	readBytes(&result, 1);
	seek(curPos);
	return result;
}

void Cbg96FileSreamer::flush()
{
	m_modem.stream.flush();
}

void Cbg96FileSreamer::startFileStream(const char* fileName, uint32_t fileSize)
{
	m_fileSizeInBytes = m_modem.modemGetFwfileLength();
	m_fileHandle = m_modem.modemOpenFwfile();
//	peek();
}

void Cbg96FileSreamer::stopFileStream()
{
	if (m_fileHandle >= 0)
	{
		m_modem.modemCloseFile(m_fileHandle);
		m_fileSizeInBytes = 0;
		m_fileHandle = -1;
		Serial.println(
				"Stop_Filestream:" + String(m_fileSizeInBytes)
						+ String(m_fileHandle));
	}
}

void Cbg96FileSreamer::spaceinfo()
{
	m_modem.sendAT(GF("+QFLDS=\"UFS\""));

	if (m_modem.waitResponse(GF("+QFLDS:")) != 1)
	{
		const uint32_t para1 = m_modem.stream.readStringUntil(' ').toInt();
		const uint32_t para2 = m_modem.stream.readStringUntil(',').toInt();
		Serial.println("FreeSpace in bytes" + String(para1) + String(para2));
	}
	else
	{
		Serial.println("QFLSD:Error");
	}
}

bool TinyGsmBG96::waitForAutoAttach()
{
	for (uint32_t trial = 0; trial < 20; trial++)
	{
		sendAT(GF("+COPS=0"));
		if (waitResponse(4000L) == 1)
		{
			return true;
		}
		delay(1000);
	}
	return false;
}

bool TinyGsmBG96::doNetworkAttach()
{
	waitForAutoAttach();
	if (!isNetworkConnected())
	{
		sendAT(GF("+QCFG=\"nwscanseq\",020301,1")); // Deactivate the bearer context
		if (waitResponse(4000L) != 1)
			return false;

		sendAT(GF("+CFUN=1,0"));
		if (waitResponse(4000L) != 1)
			return false;

		sendAT(GF("+COPS=0"));
		if (waitResponse(10000L) != 1)
			return false;

		sendAT(GF("+CTZU=1"));
		if (waitResponse(1000L) != 1)
			return false;
	}
	return true;
}

bool TinyGsmBG96::startPacketDataCall()
{
	if (false == isNetworkConnected())
	{
		return false;
	}

	if (!isIPConnected())
	{
		sendAT(GF(NETWORK_APN));
		if (waitResponse(40000L) != 1)
			return false;

		sendAT(GF("+QIACT=1"));
		if (waitResponse(40000L) != 1)
			return false;

		if (!isIPConnected())
		{
			return false;
		}
		sendAT(GF("+CTZU=1"));
		if (waitResponse(1000L) != 1)
			return false;

		sendAT(GF("+QHTTPCFG=\"contextid\",1"));
		if (waitResponse(40000L) != 1)
			return false;

		sendAT(GF("+QHTTPCFG=\"contenttype\",1"));
		if (waitResponse() != 1)
			return false;

		setSslConfig();
	}

	return true;
}

bool TinyGsmBG96::setSslConfig()
{
	/*
	 AT+QSSLCFG="sslversion",1,4
	 AT+QSSLCFG="ciphersuite",1,0xFFFF
	 AT+QSSLCFG="negotiatetime",1
	 AT+QSSLCFG="seclevel",1,0
	 */

	sendAT(GF("+QSSLCFG=\"sslversion\",1,4"));
	if (waitResponse() != 1)
		return false;

	sendAT(GF("+QSSLCFG=\"ciphersuite\",1,0xFFFF"));
	if (waitResponse() != 1)
		return false;

	sendAT(GF("+QSSLCFG=\"negotiatetime\",1"));
	if (waitResponse() != 1)
		return false;

	sendAT(GF("+QSSLCFG=\"seclevel\",1,0"));
	if (waitResponse() != 1)
		return false;

	sendAT(GF("+QHTTPCFG=\"sslctxid\",1"));
	if (waitResponse() != 1)
		return false;

	return true;
}


bool TinyGsmBG96::sendHttp_POST_Request(const char* pDataHeader,
		const char* pDataPayload, const bool disableResponseHeader,
		const bool responseIsFile)
{
	Serial.println("BG96 httpPOST:");
	if (!isNetworkConnected())
	{
		return false;
	}

	if (!isIPConnected())
	{
		return false;
	}


	if (responseIsFile || disableResponseHeader)
	{
		sendAT(GF("+QHTTPCFG=\"responseheader\",0"));
	}
	else
	{
		sendAT(GF("+QHTTPCFG=\"responseheader\",1"));
	}

	if (waitResponse(40000L) != 1)
		return false;


	{
		// header
		const uint16_t headerSize = strlen(pDataHeader);
		sendAT(GF("+QHTTPURL=" + String(headerSize) + ",80"));
		if (waitResponse(40000L, GFP(GSM_CONNECT)) != 1)
			return false;

		Serial.println(pDataHeader);
		stream.print(pDataHeader);
		if (waitResponse(40000L) != 1)
			return false;
	}


	{
		// Payload and Response
		const uint16_t payloadSize = strlen(pDataPayload);
		sendAT(GF("+QHTTPPOST=" + String(payloadSize) + ",80,125"));

		if (waitResponse(85000L, "CONNECT", "+CME ERROR:") != 1)
			return false;
		stream.write(pDataPayload, payloadSize);
		{
			Stream& serialStream = static_cast<Stream&>(Serial);
			serialStream.write(pDataPayload, payloadSize);
		}
		if (waitResponse(85000L, GSM_OK, "+CME ERROR:") != 1)
			return false;
	}

	const int32_t postResp = waitResponse(125000L, "+QHTTPPOST: 0,200", "+QHTTPPOST: 7");
	if (postResp != 1)
		return false;

//	stream.flush();

	if (responseIsFile)
	{
		sendAT("QHTTPREADFILE=\"ufs:post.txt\", 80");
		 if (waitResponse(40000L) != 1)
			 return false;
		/* Don't handle the response here */
		// if (waitResponse(40000L) != 1)
		//     return false;
	}
	else
	{
		sendAT(GF("+QHTTPREAD=80"));

		if (waitResponse(40000L, GFP(GSM_CONNECT)) != 1)
			return false;
	}

	return true;
}

bool TinyGsmBG96::sendHttp_GET_Request(const char* pDataHeader,
		const bool disableResponseHeader, const bool responseIsFile)
{
	Serial.println("BG96 httpGET:");
	if (!isNetworkConnected())
	{
		return false;
	}

	if (!isIPConnected())
	{
		return false;
	}


	if (responseIsFile || disableResponseHeader)
	{
		sendAT(GF("+QHTTPCFG=\"responseheader\",0"));
	}
	else
	{
		sendAT(GF("+QHTTPCFG=\"responseheader\",1"));
	}

	if (waitResponse(40000L) != 1)
		return false;


	uint16_t size = strlen(pDataHeader);
	sendAT(GF("+QHTTPURL=" + String(size) + ",80"));
	if (waitResponse(40000L, GFP(GSM_CONNECT)) != 1)
		return false;

	Serial.println(pDataHeader);
	stream.print(pDataHeader);
	if (waitResponse(40000L) != 1)
		return false;

	// Todo: send the GET header packet here

	sendAT(GF("+QHTTPGET=80"));
	if (waitResponse() != 1)
		return false;

	if (waitResponse(40000L, "+QHTTPGET: 0,200,") != 1)
		return false;

//	stream.flush();
	waitResponse(4000L);

	if (responseIsFile)
	{
		sendAT("+QFDEL=\"*\"");
		if (waitResponse(4000L) != 1)
			return false;
		sendAT("+QHTTPREADFILE=\"ufs:zfw.bin\",80");
		if (waitResponse(4000L) != 1)
			return false;
		if (waitResponse(330000L, "+QHTTPREADFILE: 0") != 1)
			return false;
		waitResponse(4000L);
		/* Don't handle the response here */
		// if (waitResponse(40000L) != 1)
		//     return false;
	}
	else
	{
		sendAT(GF("+QHTTPREAD=80"));
		 if (waitResponse(40000L, "CONNECT") != 1)
			 return false;
		/* Don't handle the response here */
		// if (waitResponse(40000L) != 1)
		//     return false;
//		if (waitResponse(40000L) != 1)
//			return false;
	}

	return true;
}

bool TinyGsmBG96::getStoredDeviceInfo(char* deviceIdUFSFileName,
		String& deviceName, String& env, uint8_t* pDevInfosize,
		uint16_t* pCacertsize, uint16_t* pCertsize, uint16_t* pkeysize)
{

	Serial.println("getStoredDeviceInfo");
	uint16_t fileExistsBm = 0;
	sendAT(GF("+QFLST=\"*\""));

	delay(100);
	char fileName[4][13] =
	{ "cert.pem", "deviceid.txt", "pkey.pem", "rootca.crt" };

	for (int idx = 0; idx < 4;)
	{
//			Serial.println(idx);
//			Serial.println(">");
		String line = stream.readStringUntil('\n');

		Serial.println(line);
//			Serial.println(fileName[idx]);
		if (line.indexOf(String(fileName[idx])) != -1)
		{
			fileExistsBm |= (1 << idx);
			idx++;
		}
//			Serial.println("<");
	}
	Serial.print("Found:");
	Serial.println(fileExistsBm);
	if (fileExistsBm != 0xF)
	{
		return false;
	}

	sendAT(GF("+QFDWL=\"" + String(deviceIdUFSFileName) + "\""));
	Serial.println("+QFDWL=\"" + String(deviceIdUFSFileName) + "\"");
	String line = "";
	delay(100);
	while (stream.available() && (line.indexOf('[') != 0))
	{
		line = stream.readStringUntil('\n');
	}

	const bool foundData = (line.indexOf('[') == 0);
	Serial.println("**");
	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		deviceName = line.c_str();
	}
	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		env = line.c_str();
	}

	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		*pDevInfosize = atoi(line.c_str());
	}
	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		*pCacertsize = atoi(line.c_str());
	}

	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		*pCertsize = atoi(line.c_str());
	}

	{
		line = stream.readStringUntil('\n');
		Serial.println(line);
		*pkeysize = atoi(line.c_str());
	}
	waitResponse(200);
	delay(1000);
	stream.flush();

	return foundData;
}

void TinyGsmBG96::uploadFile(char* fileName, Stream& data, uint16_t len)
{
	Serial.println("uploadfile:");
	Serial.println(GF("+QFDEL=\"" + String(fileName) + "\""));
	sendAT(GF("+QFDEL=\"" + String(fileName) + "\""));
	waitResponse(40000L);
	stream.flush();

	Serial.println(
			GF("+QFUPL=\"" + String(fileName) + "\"," + String(len) + ",100"));
	sendAT(GF("+QFUPL=\"" + String(fileName) + "\"," + String(len) + ",100"));

	Serial.println("[");
	while (data.available())
	{
		const char ch = data.read();
		Serial.print(ch);
		stream.write(ch);
	}
	Serial.println("]");
	delay(100);
	Serial.println("{");
	while (stream.available())
	{
		const char ch = data.read();
		Serial.print(ch);
		Serial.write(stream.read());
	}
	Serial.println("}");
	stream.flush();
}


bool TinyGsmBG96::syncNtpTime(time_t& epochTime)
{
	Serial.println("BG96 syncNtpTime:");
	if (!doNetworkAttach())
	{
		return false;
	}

	if (!startPacketDataCall())
	{
		return false;
	}

	sendAT(GF("+QNTP=1,\"time.google.com\",123,1"));
	if (waitResponse(120000L, "+QNTP: 0,\"") != 1)
		return false;

	tm dataAndTime;
	dataAndTime.tm_year = stream.readStringUntil('/').toInt() - 1900;
	dataAndTime.tm_mon  = stream.readStringUntil('/').toInt() - 1;
	dataAndTime.tm_mday = stream.readStringUntil(',').toInt();
	dataAndTime.tm_hour = stream.readStringUntil(':').toInt();
	dataAndTime.tm_min  = stream.readStringUntil(':').toInt();
	dataAndTime.tm_sec   = 0;
	dataAndTime.tm_isdst = 0;
	dataAndTime.tm_wday  = 0;
	dataAndTime.tm_yday  = 0;
	printf("GST: %s", asctime(&dataAndTime));

	epochTime = mktime(&dataAndTime);
	stream.flush();
	return true;
}

bool TinyGsmBG96::testAt()
{

	if (testAT(1000))
	{
		Serial.println("AT: OK");
		return true;
	}
	else
	{
		Serial.println("AT: NOK");
		return false;
	}

}

bool TinyGsmBG96::mqttSend(const char* pData, const char* deviceId,
		const char* env)
{
	Serial.println("BG96 mqttSend:");
	testAt();
	Serial.println(GF("+QCFG=\"nwscanseq\",010203,1"));
	sendAT(GF("+QCFG=\"nwscanseq\",010203,1")); // Deactivate the bearer context
	if (waitResponse(40000L) != 1)
		return false;

	sendAT(GF("+CFUN=1,0"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println("+CFUN=1,0");
	sendAT(GF("+CFUN=1,0"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+COPS=0"));
	sendAT(GF("+COPS=0"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+CTZU=1"));
	sendAT(GF("+CTZU=1"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+CGREG?"));
	sendAT(GF("+CGREG?"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+CTZU=1"));
	sendAT(GF("+CTZU=1"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF(NETWORK_APN));
	sendAT(GF(NETWORK_APN));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QIACT=1"));
	sendAT(GF("+QIACT=1"));
	if (waitResponse(4000000L) != 1)
		return false;

	Serial.println(GF("+QIACT?"));
	sendAT(GF("+QIACT?"));
	if (waitResponse(400000L) != 1)
		return false;

	Serial.println(GF("+QMTCFG=\"SSL\", 0, 1, 2"));
	sendAT(GF("+QMTCFG=\"SSL\", 0, 1, 2"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QMTCFG=\"SSL\", 0, 1, 1"));
	sendAT(GF("+QMTCFG=\"SSL\", 0, 1, 1"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"cacert\",1,\"ufs:cacert.pem\""));
	sendAT(GF("+QSSLCFG=\"cacert\",1,\"ufs:cacert.pem\""));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"clientcert\",1,\"ufs:clientcert.pem\""));
	sendAT(GF("+QSSLCFG=\"clientcert\",1,\"ufs:clientcert.pem\""));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"clientkey\",1,\"ufs:clientkey.pem\""));
	sendAT(GF("+QSSLCFG=\"clientkey\",1,\"ufs:clientkey.pem\""));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"seclevel\",2,2"));
	sendAT(GF("+QSSLCFG=\"seclevel\",2,2"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"sslversion\",2,4"));
	sendAT(GF("+QSSLCFG=\"sslversion\",2,4"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"ciphersuite\",2,0XFFFF"));
	sendAT(GF("+QSSLCFG=\"ciphersuite\",2,0XFFFF"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(GF("+QSSLCFG=\"ignorelocaltime\",1"));
	sendAT(GF("+QSSLCFG=\"ignorelocaltime\",1"));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(
			GF(
					"+QMTOPEN=0,\"ay9x8s2gs2y1g-ats.iot.us-west-2.amazonaws.com\",8883"));
	sendAT(
			GF(
					"+QMTOPEN=0,\"ay9x8s2gs2y1g-ats.iot.us-west-2.amazonaws.com\",8883"));
	if (waitResponse(40000L, "+QMTOPEN: 0,0") != 1)
		return false;

	Serial.println(
			GF("+QMTCONN=0,\"" + String(deviceId) + "-" + String(env) + "\""));
	sendAT(GF("+QMTCONN=0,\"" + String(deviceId) + "-" + String(env) + "\""));
	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(
			GF(
					"+QMTSUB=0,1,\"hivegenie/" + String(env) + "/"
							+ String(deviceId) + "/data\",1"));
	sendAT(
			GF(
					"+QMTSUB=0,1,\"hivegenie/" + String(env) + "/"
							+ String(deviceId) + "/data\",1"));

	if (waitResponse(40000L) != 1)
		return false;

	Serial.println(
			GF(
					"+QMTPUB=0,1,1,0,\"hivegenie/" + String(env) + "/"
							+ String(deviceId) + "/data\",5"));
	sendAT(
			GF(
					"+QMTPUB=0,1,1,0,\"hivegenie/" + String(env) + "/"
							+ String(deviceId) + "/data\",5"));
	if (waitResponse(40000L) != 1)
		return false;

	return true;
}
