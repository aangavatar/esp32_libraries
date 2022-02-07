/**
 * @file       TinyGsmClientBG96.h
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Apr 2018
 */

#ifndef TinyGsmClientBG96_h
#define TinyGsmClientBG96_h
//#pragma message("TinyGSM:  TinyGsmClientBG96")

//#define TINY_GSM_DEBUG Serial
//#define TINY_GSM_USE_HEX

#define TINY_GSM_MUX_COUNT 12
#include "TinyGsmCommon.h"

#define GSM_NL "\r\n"
static const char GSM_OK[] TINY_GSM_PROGMEM = "OK" GSM_NL;
static const char GSM_ERROR[] TINY_GSM_PROGMEM = "ERROR" GSM_NL;
static const char GSM_CONNECT[] TINY_GSM_PROGMEM = "CONNECT" GSM_NL;

#if 0
#if HG_TEST_MODE
#define NETWORK_APN "+QICSGP=1,1,\"airtelgprs.com\",\"\",\"\",1"
#else
//#define NETWORK_APN "+QICSGP=1,1,\"airtelgprs.com\",\"\",\"\",1"
#define NETWORK_APN "+QICSGP=1,1,\"vzwinternet\",\"\",\"\",1"
				    "+QICSGP=1,1,\"vzwinternet1\",\"\",\"\",1"
#endif
#endif

enum SimStatus
{
	SIM_ERROR = 0, SIM_READY = 1, SIM_LOCKED = 2,
};

enum RegStatus
{
	REG_UNREGISTERED = 0,
	REG_SEARCHING = 2,
	REG_DENIED = 3,
	REG_OK_HOME = 1,
	REG_OK_ROAMING = 5,
	REG_UNKNOWN = 4,
};

class TinyGsmBG96: public TinyGsmModem
{

public:

	class GsmClient: public Client
	{
		friend class TinyGsmBG96;
		typedef TinyGsmFifo<uint8_t, TINY_GSM_RX_BUFFER> RxFifo;

	public:
		GsmClient()
		{
		}

		GsmClient(TinyGsmBG96& modem, uint8_t mux = 1)
		{
			init(&modem, mux);
		}

		bool init(TinyGsmBG96* modem, uint8_t mux = 1)
		{
			this->at = modem;
			this->mux = mux;
			sock_available = 0;
			sock_connected = false;
			got_data = false;
			at->sockets[mux] = this;
			return true;
		}

	public:
		virtual int connect(const char *host, uint16_t port);

		virtual int connect(IPAddress ip, uint16_t port, int timeout);

		virtual int connect(const char *host, uint16_t port, int timeout);

		virtual int connect(IPAddress ip, uint16_t port)
		{
			String host;
			host.reserve(16);
			host += ip[0];
			host += ".";
			host += ip[1];
			host += ".";
			host += ip[2];
			host += ".";
			host += ip[3];
			return connect(host.c_str(), port);
		}

		virtual void stop()
		{
			TINY_GSM_YIELD()
			;
			// Read and dump anything remaining in the modem's internal buffer.
			// The socket will appear open in response to connected() even after it
			// closes until all data is read from the buffer.
			// Doing it this way allows the external mcu to find and get all of the data
			// that it wants from the socket even if it was closed externally.
			rx.clear();
			at->maintain();
			while (sock_available > 0)
			{
				sock_available -= at->modemRead(
						TinyGsmMin((uint16_t) rx.free(), sock_available), mux);
				rx.clear();
				at->maintain();
			}
			at->sendAT(GF("+QICLOSE="), mux);
			sock_connected = false;
			at->waitResponse();
		}

		virtual size_t write(const uint8_t *buf, size_t size)
		{
			TINY_GSM_YIELD()
			;
			at->maintain();
			return at->modemSend(buf, size, mux);
		}

		virtual size_t write(uint8_t c)
		{
			return write(&c, 1);
		}

		virtual size_t write(const char *str)
		{
			if (str == NULL)
				return 0;
			return write((const uint8_t *) str, strlen(str));
		}

		virtual int available()
		{
			TINY_GSM_YIELD()
			;
			if (!rx.size())
			{
				at->maintain();
			}
			return rx.size() + sock_available;
		}

		virtual int read(uint8_t *buf, size_t size)
		{
			TINY_GSM_YIELD()
			;
			at->maintain();
			size_t cnt = 0;
			while (cnt < size)
			{
				size_t chunk = TinyGsmMin(size - cnt, rx.size());
				if (chunk > 0)
				{
					rx.get(buf, chunk);
					buf += chunk;
					cnt += chunk;
					continue;
				}
				// TODO: Read directly into user buffer?
				at->maintain();
				if (sock_available > 0)
				{
					sock_available -= at->modemRead(
							TinyGsmMin((uint16_t) rx.free(), sock_available),
							mux);
				}
				else
				{
					break;
				}
			}
			return cnt;
		}

		virtual int read()
		{
			uint8_t c;
			if (read(&c, 1) == 1)
			{
				return c;
			}
			return -1;
		}

		virtual int peek()
		{
			return -1;
		} //TODO
		virtual void flush()
		{
			at->stream.flush();
		}

		virtual uint8_t connected()
		{
			if (available())
			{
				return true;
			}
			return sock_connected;
		}
		virtual operator bool()
		{
			return connected();
		}

		/*
		 * Extended API
		 */

		String remoteIP() TINY_GSM_ATTR_NOT_IMPLEMENTED;

	private:
		TinyGsmBG96* at;
		uint8_t mux;
		uint16_t sock_available;
		bool sock_connected;
		bool got_data;
		RxFifo rx;
		String _apn;
	};

// class GsmClientSecure : public GsmClient
// {
// public:
//   GsmClientSecure() {}
//
//   GsmClientSecure(TinyGsmBG96& modem, uint8_t mux = 1)
//     : GsmClient(modem, mux)
//   {}
//
// public:
//   virtual int connect(const char *host, uint16_t port) {
//     stop();
//     TINY_GSM_YIELD();
//     rx.clear();
//     sock_connected = at->modemConnect(host, port, mux, true);
//     return sock_connected;
//   }
// };

public:

	TinyGsmBG96(Stream& stream) :
			TinyGsmModem(stream), stream(stream)
	{
		memset(sockets, 0, sizeof(sockets));
	}

	/*
	 * Basic functions
	 */

	bool init(const char* pin = NULL)
	{
		DBG(GF("### TinyGSM Version:"), TINYGSM_VERSION);
		if (!testAT())
		{
			return false;
		}
		sendAT(GF("&FZE0"));  // Factory + Reset + Echo Off
		if (waitResponse() != 1)
		{
			return false;
		} DBG(GF("### Modem:"), getModemName());
		getSimStatus();
		return true;
	}

	String getModemName()
	{
		return "Quectel BG96";
	}

	void setBaud(unsigned long baud)
	{
		sendAT(GF("+IPR="), baud);
	}

	bool testAT(unsigned long timeout = 10000L)
	{
		for (unsigned long start = millis(); millis() - start < timeout;)
		{
			sendAT(GF(""));
			if (waitResponse(200) == 1)
				return true;
			delay(100);
		}
		return false;
	}

	void maintain()
	{
		for (int mux = 0; mux < TINY_GSM_MUX_COUNT; mux++)
		{
			GsmClient* sock = sockets[mux];
			if (sock && sock->got_data)
			{
				sock->got_data = false;
				sock->sock_available = modemGetAvailable(mux);
			}
		}
		while (stream.available())
		{
			waitResponse(10, NULL, NULL);
		}
	}

	bool factoryDefault()
	{
		sendAT(GF("&FZE0&W"));  // Factory + Reset + Echo Off + Write
		waitResponse();
		sendAT(GF("+IPR=0"));   // Auto-baud
		waitResponse();
		sendAT(GF("&W"));       // Write configuration
		return waitResponse() == 1;
	}

	String getModemInfo()
	{
		sendAT(GF("I"));
		String res;
		if (waitResponse(1000L, res) != 1)
		{
			return "";
		}
		res.replace(GSM_NL "OK" GSM_NL, "");
		res.replace(GSM_NL, " ");
		res.trim();
		return res;
	}

	bool hasSSL()
	{
		return false;  // TODO: For now
	}

	bool hasWifi()
	{
		return false;
	}

	bool hasGPRS()
	{
		return true;
	}

	/*
	 * Power functions
	 */

	bool restart()
	{
		if (!testAT())
		{
			return false;
		}
		sendAT(GF("+CFUN=1,1"));
		if (waitResponse(60000L, GF("POWERED DOWN")) != 1)
		{
			return false;
		}
		delay(3000);
		return init();
	}

	bool poweroff()
	{
		sendAT(GF("+QPOWD=1"));
//		waitResponse(300);  // returns OK first
		return (waitResponse(34000, GF("POWERED DOWN")) == 1);
	}

	bool radioOff()
	{
		sendAT(GF("+CFUN=0"));
		if (waitResponse(10000L) != 1)
		{
			return false;
		}
		delay(3000);
		return true;
	}

	/*
	 * SIM card functions
	 */

	bool simUnlock(const char *pin)
	{
		sendAT(GF("+CPIN=\""), pin, GF("\""));
		return waitResponse() == 1;
	}

	String getSimCCID()
	{
		sendAT(GF("+ICCID"));
		if (waitResponse(GF(GSM_NL "+ICCID:")) != 1)
		{
			return "";
		}
		String res = stream.readStringUntil('\n');
		waitResponse();
		res.trim();
		return res;
	}

	String getIMEI()
	{
		sendAT(GF("+GSN"));
		if (waitResponse(GF(GSM_NL)) != 1)
		{
			return "";
		}
		String res = stream.readStringUntil('\n');
		waitResponse();
		res.trim();
		return res;
	}

	SimStatus getSimStatus(unsigned long timeout = 10000L)
	{
		for (unsigned long start = millis(); millis() - start < timeout;)
		{
			sendAT(GF("+CPIN?"));
			if (waitResponse(GF(GSM_NL "+CPIN:")) != 1)
			{
				delay(1000);
				continue;
			}
			int status = waitResponse(GF("READY"), GF("SIM PIN"), GF("SIM PUK"),
					GF("NOT INSERTED"));
			waitResponse();
			switch (status)
			{
			case 2:
			case 3:
				return SIM_LOCKED;
			case 1:
				return SIM_READY;
			default:
				return SIM_ERROR;
			}
		}
		return SIM_ERROR;
	}

	RegStatus getRegistrationStatus()
	{
		sendAT(GF("+CREG?"));
		if (waitResponse(GF(GSM_NL "+CREG:")) != 1)
		{
			return REG_UNKNOWN;
		}
		if (!streamSkipUntil(','))
		{
			return REG_UNKNOWN;
		}
		const RegStatus status = static_cast<RegStatus>(stream.readStringUntil('\n').toInt());
		waitResponse();
		return status;
	}

	String getOperator()
	{
		sendAT(GF("+COPS?"));
		if (waitResponse(GF(GSM_NL "+COPS:")) != 1)
		{
			return "";
		}
		streamSkipUntil('"'); // Skip mode and format
		String res = stream.readStringUntil('"');
		waitResponse();
		return res;
	}

	struct cellInfo_t
	{
		uint32_t cellId;
		uint32_t lac;
		uint32_t mcc;
		uint32_t mnc;
		int32_t sigStrength;
		uint32_t ta;
	};

	struct cellList_t
	{
		cellInfo_t cellInfo[4];
	};

	inline uint32_t convertHex2Dec(String hexString)
	{
		return strtol(hexString.c_str(), NULL, 16);
	}

	void skipNumTerminator(const char terminator, const uint32_t times)
	{
		for (uint32_t i = 0; i < times; i++)
		{
			streamSkipUntil(terminator);
		}
	}

	int32_t fillServingCellInfo(cellInfo_t& sCellInfo, String& rat)
	{
		/*	  Response In the case of GSM mode:
		 +QENG:  "servingscell",<state>,"GSM",<mcc>,<mnc>,<lac>,<cellid>,<bsic>,<arfcn>,<band>,<rxlev>,<txp>,<rla>,<drx>,<c1>,<c2>,<gprs>,<tch>,<ts>,<ta>,<maio>,<hsn>,<rxlevsub>,<rxlevfull>,<rxqualsub>,<rxqualfull>,<voicecodec>
		 OK

		 In case of LTECatM1/CatNB1mode:+QENG:
		 "servingcell",<state>,"rat",<is_tdd>,<mcc>,<mnc>,<cellid>,<pcid>,<earfcn>,<freq_band_ind>,<ul_bandwidth>,<dl_bandwidth>,<tac>,<rsrp>,<rsrq>,<rssi>,<sinr>,<srxlev>
		 OK
		 */
		sendAT(GF("+QENG=\"servingcell\""));


		if (waitResponse(GF("+QENG: \"servingcell\",\"NOCONN\",")) != 1)
		{
			waitResponse();
			return -1;
		}

		rat = stream.readStringUntil(',');
		if (rat == "")
		{
			waitResponse();
			return -2;
		}

		bool isRatTypeGsm = (rat == "\"GSM\"");

		Serial.println("RAT:" + String(rat));

		if (isRatTypeGsm)
		{
			String temp = stream.readStringUntil(',');
			sCellInfo.mcc = temp.toInt();
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.mnc = temp.toInt();
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.lac = convertHex2Dec(temp);
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.cellId = convertHex2Dec(temp);
			Serial.println(temp);

			skipNumTerminator(',', 3);
			temp = stream.readStringUntil(',');
			sCellInfo.sigStrength = temp.toInt();
			Serial.println(temp);

			skipNumTerminator(',', 8);
			sCellInfo.ta = 0; // stream.readStringUntil(',').toInt();
		}
		else
		{
			skipNumTerminator(',', 1);
			sCellInfo.mcc = stream.readStringUntil(',').toInt();
			sCellInfo.mnc = stream.readStringUntil(',').toInt();
			sCellInfo.cellId =  convertHex2Dec(stream.readStringUntil(','));
			skipNumTerminator(',', 5);
			sCellInfo.lac =  convertHex2Dec(stream.readStringUntil(','));
			sCellInfo.sigStrength = stream.readStringUntil(',').toInt();
		}

		waitResponse();
		return 1;
	}

	int32_t fillNeighborCellInfo(cellList_t& cellList, String& rat)
	{
#if 0
		/*	  Response In the case of GSM mode:
		 +QENG:  "servingscell",<state>,"GSM",<mcc>,<mnc>,<lac>,<cellid>,<bsic>,<arfcn>,<band>,<rxlev>,<txp>,<rla>,<drx>,<c1>,<c2>,<gprs>,<tch>,<ts>,<ta>,<maio>,<hsn>,<rxlevsub>,<rxlevfull>,<rxqualsub>,<rxqualfull>,<voicecodec>
		 OK

		 In case of LTECatM1/CatNB1mode:+QENG:
		 "servingcell",<state>,"rat",<is_tdd>,<mcc>,<mnc>,<cellid>,<pcid>,<earfcn>,<freq_band_ind>,<ul_bandwidth>,<dl_bandwidth>,<tac>,<rsrp>,<rsrq>,<rssi>,<sinr>,<srxlev>
		 OK
		 */
		sendAT(GF("+QENG=\"neighbourcell\""));


		if (waitResponse(GF("+QENG: \"neighbourcell")) != 1)
		{
			return -1;
		}

		rat = stream.readStringUntil(',');
		if (rat == "")
		{
			return -2;
		}

		bool isRatTypeGsm = (rat == "\"GSM\"");

		Serial.println("RAT:" + String(rat));

		if (isRatTypeGsm)
		{
			String temp = stream.readStringUntil(',');
			sCellInfo.mcc = temp.toInt();
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.mnc = temp.toInt();
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.lac = convertHex2Dec(temp);
			Serial.println(temp);

			temp = stream.readStringUntil(',');
			sCellInfo.cellId = convertHex2Dec(temp);
			Serial.println(temp);

			skipNumTerminator(',', 3);
			temp = stream.readStringUntil(',');
			sCellInfo.sigStrength = temp.toInt();
			Serial.println(temp);

			skipNumTerminator(',', 8);
			sCellInfo.ta = 0; // stream.readStringUntil(',').toInt();
		}
		else
		{
			skipNumTerminator(',', 1);
			sCellInfo.mcc = stream.readStringUntil(',').toInt();
			sCellInfo.mnc = stream.readStringUntil(',').toInt();
			sCellInfo.cellId =  convertHex2Dec(stream.readStringUntil(','));
			skipNumTerminator(',', 5);
			sCellInfo.lac =  convertHex2Dec(stream.readStringUntil(','));
			sCellInfo.sigStrength = stream.readStringUntil(',').toInt();
		}
		return 1;
#else
		return 0;
#endif
	}
	/*
	 * Generic network functions
	 */

	int16_t getSignalQuality()
	{
		sendAT(GF("+CSQ"));
		if (waitResponse(GF(GSM_NL "+CSQ:")) != 1)
		{
			return 99;
		}
		int res = stream.readStringUntil(',').toInt();
		waitResponse();
		return res;
	}

	bool isNetworkConnected()
	{
		// RegStatus s = getRegistrationStatus(); // Not needed as we check the IP only directly
		return true; //(s == REG_OK_HOME || s == REG_OK_ROAMING);
	}

	void captureOutput_debug()
	{
		delay(1000);
		while (Serial1.available())
		{
			Serial.print((char) Serial1.read());
		}
	}

	bool isIPConnected()
	{
		sendAT(GF("+QIACT?"));
		if (waitResponse(GF("+QIACT: 1,"), GSM_OK) != 1)
			return false;

		uint32_t contextState = readStringUntil(',').toInt();
		if (contextState == 1)
		{
			skipNumTerminator(',', 1);

			String localIp = readStringUntil('\n');
			Serial.print("localIp:");
			Serial.println(localIp);
			Serial.flush();
		}
		else
		{
			return false;
		}

		if (waitResponse() != 1)
			return false;

		return (contextState == 1);
	}

	/*
	 * GPRS functions
	 */
	bool gprsConnect(const char* apn, const char* user = NULL, const char* pwd =
			NULL)
	{
		gprsDisconnect();

		//Configure the TCPIP Context
		sendAT(GF("+QICSGP=1,1,\""), apn, GF("\",\""), user, GF("\",\""), pwd,
				GF("\""));
		if (waitResponse() != 1)
		{
			return false;
		}

		//Activate GPRS/CSD Context
		sendAT(GF("+QIACT=1"));
		if (waitResponse(150000L) != 1)
		{
			return false;
		}

		//Attach to Packet Domain service - is this necessary?
		sendAT(GF("+CGATT=1"));
		if (waitResponse(60000L) != 1)
		{
			return false;
		}

		return true;
	}

	bool doNetworkAttach();

	bool waitForAutoAttach();

	bool updateApn();

	bool startPacketDataCall();

	bool setSslConfig();

	bool sendHttp_GET_Request(const char* pDataHeader, const bool disableResponseHeader = false, const bool responseIsFile = false);

	bool sendHttp_POST_Request(const char* pDataHeader, const char* pDataPayload, const bool disableResponseHeader = false, const bool responseIsFile = false);

	bool getStoredDeviceInfo(char* deviceIdUFSFileName, String& deviceName,
			String& env, uint8_t* pDevInfosize, uint16_t* pCacertsize,
			uint16_t* pCertsize, uint16_t* pkeysize);

	void uploadFile(char* fileName, Stream& data, uint16_t len);

	bool syncNtpTime(time_t& epochTime);

	bool testAt();

	bool mqttSend(const char* pData, const char* deviceId, const char* env);

	bool gprsDisconnect()
	{
		sendAT(GF("+QIDEACT=1"));  // Deactivate the bearer context
		if (waitResponse(40000L) != 1)
			return false;

		return true;
	}

	bool isGprsConnected()
	{
		sendAT(GF("+CGATT?"));
		if (waitResponse(GF(GSM_NL "+CGATT:")) != 1)
		{
			return false;
		}
		int res = stream.readStringUntil('\n').toInt();
		waitResponse();
		if (res != 1)
			return false;

		return localIP() != IPAddress(0, 0, 0, 0);
	}

	inline String readStringUntil(char terminator)
	{
		return stream.readStringUntil(terminator);
	}

	size_t readBytes(char *buffer, size_t length)
	{
		return stream.readBytes(buffer, length);

	}

	inline size_t readBytesUntil(char terminator, uint8_t *buffer,
			size_t length)
	{
		return stream.readBytesUntil(terminator, buffer, length);
	}
	/*
	 * IP Address functions
	 */

	String getLocalIP()
	{
		sendAT(GF("+QILOCIP"));
		stream.readStringUntil('\n');
		String res = stream.readStringUntil('\n');
		if (waitResponse() != 1)
		{
			return "";
		}
		return res;
	}

	/*
	 * Phone Call functions
	 */

	bool setGsmBusy(bool busy = true) TINY_GSM_ATTR_NOT_AVAILABLE;

	bool callAnswer()
	{
		sendAT(GF("A"));
		return waitResponse() == 1;
	}

	// Returns true on pick-up, false on error/busy
	bool callNumber(const String& number) TINY_GSM_ATTR_NOT_IMPLEMENTED;

	bool callHangup()
	{
		sendAT(GF("H"));
		return waitResponse() == 1;
	}

	// 0-9,*,#,A,B,C,D
	bool dtmfSend(char cmd, int duration_ms = 100)
	{ // TODO: check
		duration_ms = constrain(duration_ms, 100, 1000);

		sendAT(GF("+VTD="), duration_ms / 100); // VTD accepts in 1/10 of a second
		waitResponse();

		sendAT(GF("+VTS="), cmd);
		return waitResponse(10000L) == 1;
	}

	/*
	 * Messaging functions
	 */

	String sendUSSD(const String& code) TINY_GSM_ATTR_NOT_IMPLEMENTED;

	bool sendSMS(const String& number, const String& text)
	{
		sendAT(GF("+CMGF=1"));
		waitResponse();
		//Set GSM 7 bit default alphabet (3GPP TS 23.038)
		sendAT(GF("+CSCS=\"GSM\""));
		waitResponse();
		sendAT(GF("+CMGS=\""), number, GF("\""));
		if (waitResponse(GF(">")) != 1)
		{
			return false;
		}
		stream.print(text);
		stream.write((char) 0x1A);
		stream.flush();
		return waitResponse(60000L) == 1;
	}

	bool sendSMS_UTF16(const String& number, const void* text, size_t len)
	{
		sendAT(GF("+CMGF=1"));
		waitResponse();
		sendAT(GF("+CSCS=\"HEX\""));
		waitResponse();
		sendAT(GF("+CSMP=17,167,0,8"));
		waitResponse();

		sendAT(GF("+CMGS=\""), number, GF("\""));
		if (waitResponse(GF(">")) != 1)
		{
			return false;
		}

		uint16_t* t = (uint16_t*) text;
		for (size_t i = 0; i < len; i++)
		{
			uint8_t c = t[i] >> 8;
			if (c < 0x10)
			{
				stream.print('0');
			}
			stream.print(c, HEX);
			c = t[i] & 0xFF;
			if (c < 0x10)
			{
				stream.print('0');
			}
			stream.print(c, HEX);
		}
		stream.write((char) 0x1A);
		stream.flush();
		return waitResponse(60000L) == 1;
	}

	/*
	 * Location functions
	 */

	String getGsmLocation() TINY_GSM_ATTR_NOT_AVAILABLE;

	/*
	 * Battery functions
	 */

	// Use: float vBatt = modem.getBattVoltage() / 1000.0;
	uint16_t getBattVoltage()
	{
		sendAT(GF("+CBC"));
		if (waitResponse(GF(GSM_NL "+CBC:")) != 1)
		{
			return 0;
		}
		streamSkipUntil(','); // Skip
		streamSkipUntil(','); // Skip

		uint16_t res = stream.readStringUntil(',').toInt();
		waitResponse();
		return res;
	}

	int8_t getBattPercent()
	{
		sendAT(GF("+CBC"));
		if (waitResponse(GF(GSM_NL "+CBC:")) != 1)
		{
			return false;
		}
		stream.readStringUntil(',');
		int res = stream.readStringUntil(',').toInt();
		waitResponse();
		return res;
	}

	/*
	 * Client related functions
	 */

protected:

	bool modemConnect(const char* host, uint16_t port, uint8_t mux, bool ssl =
			false)
	{
		int rsp;
		// <PDPcontextID>(1-16), <connectID>(0-11),"TCP/UDP/TCP LISTENER/UDP SERVICE",
		// "<IP_address>/<domain_name>",<remote_port>,<local_port>,<access_mode>(0-2 0=buffer)
		sendAT(GF("+QIOPEN=1,"), mux, ',', GF("\"TCP"), GF("\",\""), host,
				GF("\","), port, GF(",0,0"));
		rsp = waitResponse();

		if (waitResponse(20000L, GF(GSM_NL "+QIOPEN:")) != 1)
		{
			return false;
		}

		if (stream.readStringUntil(',').toInt() != mux)
		{
			return false;
		}
		// Read status
		rsp = stream.readStringUntil('\n').toInt();

		return (0 == rsp);
	}

	int16_t modemSend(const void* buff, size_t len, uint8_t mux)
	{
		sendAT(GF("+QISEND="), mux, ',', len);
		if (waitResponse(GF(">")) != 1)
		{
			return 0;
		}
		stream.write((uint8_t*) buff, len);
		stream.flush();
		if (waitResponse(GF(GSM_NL "SEND OK")) != 1)
		{
			return 0;
		}
		// TODO: Wait for ACK? AT+QISEND=id,0
		return len;
	}

	size_t modemRead(size_t size, uint8_t mux)
	{
		sendAT(GF("+QIRD="), mux, ',', size);
		if (waitResponse(GF("+QIRD:")) != 1)
		{
			return 0;
		}
		size_t len = stream.readStringUntil('\n').toInt();

		for (size_t i = 0; i < len; i++)
		{
			while (!stream.available())
			{
				TINY_GSM_YIELD()
				;
			}
			char c = stream.read();
			sockets[mux]->rx.put(c);
		}
		waitResponse();
		return len;
	}

	size_t modemGetAvailable(uint8_t mux)
	{
		sendAT(GF("+QIRD="), mux, GF(",0"));
		size_t result = 0;
		if (waitResponse(GF("+QIRD:")) == 1)
		{
			streamSkipUntil(','); // Skip total received
			streamSkipUntil(','); // Skip have read
			result = stream.readStringUntil('\n').toInt();
			DBG("### STILL:", mux, "has", result);
			waitResponse();
		}
		if (!result)
		{
			sockets[mux]->sock_connected = modemGetConnected(mux);
		}
		return result;
	}

	bool modemGetConnected(uint8_t mux)
	{
		sendAT(GF("+QISTATE=1,"), mux);
		//+QISTATE: 0,"TCP","151.139.237.11",80,5087,4,1,0,0,"uart1"

		if (waitResponse(GF("+QISTATE:")))
			return false;

		streamSkipUntil(','); // Skip mux
		streamSkipUntil(','); // Skip socket type
		streamSkipUntil(','); // Skip remote ip
		streamSkipUntil(','); // Skip remote port
		streamSkipUntil(','); // Skip local port
		int res = stream.readStringUntil(',').toInt(); // socket state

		waitResponse();

		// 0 Initial, 1 Opening, 2 Connected, 3 Listening, 4 Closing
		return 2 == res;
	}

public:
	int32_t modemGetFwfileLength()
	{
		sendAT(GF("+QFLST=\"zfw.bin\""));
		Serial.println(GF("+QFLST=\"zfw.bin\""));
		//+QFLST: "F_M12-1.bmp",562554

		if (waitResponse(GF("+QFLST:")) != 1)
			return -2;

		streamSkipUntil(','); // Skip filename
		uint32_t size = stream.readStringUntil('\n').toInt(); // socket state

		if (waitResponse() != 1)
			return -1;

		Serial.print("FileLen:");
		Serial.println(size);
		return size;
	}

	int32_t modemOpenFwfile()
	{
		sendAT(GF("+QFOPEN=\"zfw.bin\",2")); // READ-ONLY mode
		Serial.println(GF("+QFOPEN=\"zfw.bin\",2"));
		//+QFOPEN: <filehandle>

		delay(1000);
//    while (stream.available())
//    {
//    	Serial.print((char)stream.read());
//    }
		if (waitResponse(GF("+QFOPEN:")) != 1)
			return -1;

		int32_t fileHandle = stream.readStringUntil('\n').toInt(); // socket state

		if (waitResponse() != 1)
			return -1;

		Serial.print("FileHandle:");
		Serial.println(fileHandle);
		return fileHandle;
	}

	int32_t modemReadFile(uint8_t fileHandle, uint32_t length)
	{
		sendAT(GF("+QFREAD=" + String(fileHandle) + "," + String(length))); // READ-ONLY mode
		Serial.println(
				GF("+QFREAD=" + String(fileHandle) + "," + String(length)));

//    delay(500);
//        while (stream.available())
//        {
//        	Serial.print((char)stream.read());
//        }
//        return 64;
		if (waitResponse(40000, GF("CONNECT")) != 1)
			return -1;
		stream.readStringUntil(' ');
		const uint32_t sentDataLength = stream.readStringUntil('\n').toInt(); // socket state

		Serial.print("dataLength:");
		Serial.println(sentDataLength);
//    modemCloseFile();
		return sentDataLength;
	}

	bool modemCloseFile(uint8_t fileHandle)
	{
		sendAT(GF("+QFCLOSE=" + String(fileHandle))); // READ-ONLY mode
		Serial.println(GF("+QFCLOSE=" + String(fileHandle)));

		if (waitResponse() != 1)
			return false;
		return true;
	}

	/*
	 Utilities
	 */

	template<typename ... Args>
	void sendAT(Args ... cmd)
	{
		delay(500);
		streamWrite("AT", cmd..., GSM_NL);
		stream.flush();
		TINY_GSM_YIELD()
		;
		DBG("### AT:", cmd...);
	}

	// TODO: Optimize this!
	uint8_t waitResponse2(uint32_t timeout, String& data,
			GsmConstStr r1 = GFP(GSM_OK))
	{
		/*String r1s(r1); r1s.trim();
		 String r2s(r2); r2s.trim();
		 String r3s(r3); r3s.trim();
		 String r4s(r4); r4s.trim();
		 String r5s(r5); r5s.trim();
		 DBG("### ..:", r1s, ",", r2s, ",", r3s, ",", r4s, ",", r5s);*/
		data.reserve(64);
		int index = 0;
		unsigned long startMillis = millis();
		do
		{
			TINY_GSM_YIELD()
			;
			while (stream.available() > 0)
			{
				int a = stream.read();
				if (a <= 0)
					continue; // Skip 0x00 bytes, just in case
				data += (char) a;
				Serial.print(a);
				if (r1 && data.startsWith(r1))
				{
					index = 1;
					goto finish;
				}
				else
				{
					stream.readStringUntil('\n');
				}

				data = "";
			}

		} while (millis() - startMillis < timeout);

		finish: if (!index)
		{
			data.trim();
			if (data.length())
			{
				DBG("### Unhandled:", data);
			}
			data = "";
		}
		//DBG('<', index, '>');
		return index;
	}

	// TODO: Optimize this!
	uint8_t waitResponse(uint32_t timeout, String& data,
			GsmConstStr r1 = GFP(GSM_OK), GsmConstStr r2 = GFP(GSM_ERROR),
			GsmConstStr r3 = NULL, GsmConstStr r4 = NULL, GsmConstStr r5 = NULL)
	{
		/*String r1s(r1); r1s.trim();
		 String r2s(r2); r2s.trim();
		 String r3s(r3); r3s.trim();
		 String r4s(r4); r4s.trim();
		 String r5s(r5); r5s.trim();
		 DBG("### ..:", r1s, ",", r2s, ",", r3s, ",", r4s, ",", r5s);*/
		data.reserve(64);
		int index = 0;
		unsigned long startMillis = millis();
		do
		{
			TINY_GSM_YIELD()
			;
			while (stream.available() > 0)
			{
				int a = stream.read();
				Serial.write(a);
				if (a <= 0)
					continue; // Skip 0x00 bytes, just in case
				data += (char) a;
				if (r1 && data.endsWith(r1))
				{
					index = 1;
					goto finish;
				}
				else if (r2 && data.endsWith(r2))
				{
					index = 2;
					goto finish;
				}
				else if (r3 && data.endsWith(r3))
				{
					index = 3;
					goto finish;
				}
				else if (r4 && data.endsWith(r4))
				{
					index = 4;
					goto finish;
				}
				else if (r5 && data.endsWith(r5))
				{
					index = 5;
					goto finish;
				}
				else if (data.endsWith(GF(GSM_NL "+QIURC:")))
				{
					stream.readStringUntil('\"');
					String urc = stream.readStringUntil('\"');
					stream.readStringUntil(',');
					if (urc == "recv")
					{
						int mux = stream.readStringUntil('\n').toInt();
						DBG("### URC RECV:", mux);
						if (mux >= 0 && mux < TINY_GSM_MUX_COUNT
								&& sockets[mux])
						{
							sockets[mux]->got_data = true;
						}
					}
					else if (urc == "closed")
					{
						int mux = stream.readStringUntil('\n').toInt();
						DBG("### URC CLOSE:", mux);
						if (mux >= 0 && mux < TINY_GSM_MUX_COUNT
								&& sockets[mux])
						{
							sockets[mux]->sock_connected = false;
						}
					}
					else
					{
						stream.readStringUntil('\n');
					}
					data = "";
				}
			}
		} while (millis() - startMillis < timeout);
		finish: if (!index)
		{
			data.trim();
			if (data.length())
			{
				DBG("### Unhandled:", data);
			}
			data = "";
		}
		//DBG('<', index, '>');
		return index;
	}

	uint8_t waitResponse2(uint32_t timeout, GsmConstStr r1 = GFP(GSM_OK))
	{
		String data;
		return waitResponse2(timeout, data, r1);
	}

	uint8_t waitResponse(uint32_t timeout, GsmConstStr r1 = GFP(GSM_OK),
			GsmConstStr r2 = GFP(GSM_ERROR), GsmConstStr r3 = NULL,
			GsmConstStr r4 = NULL, GsmConstStr r5 = NULL)
	{
		String data;
		return waitResponse(timeout, data, r1, r2, r3, r4, r5);
	}

	uint8_t waitResponse(GsmConstStr r1 = GFP(GSM_OK),
			GsmConstStr r2 = GFP(GSM_ERROR), GsmConstStr r3 = NULL,
			GsmConstStr r4 = NULL, GsmConstStr r5 = NULL)
	{
		return waitResponse(1000, r1, r2, r3, r4, r5);
	}


	inline void setApn(String& apn)
	{
		_apn = apn;
	}

public:
	Stream& stream;

protected:
	GsmClient* sockets[TINY_GSM_MUX_COUNT];
	String _apn;
};

class Cbg96FileSreamer: public Stream
{
private:
	uint32_t m_fileSizeInBytes;
	uint32_t m_currBufferTop;
	uint32_t m_currBufferBottom;
	TinyGsmBG96& m_modem;
	int8_t m_fileHandle;
	uint32_t getCurrentPosition();

public:

	enum FS_CBg96Ota_t
	{
		e_OTADONE = 0,
		e_INVALID_FH = -1,
		e_NOTFINISHOTA = -2,
		e_UPDERROR = -3,
		e_NOSPACE = -4,
		e_SIZE_INVALID = -5,

	};
	Cbg96FileSreamer(TinyGsmBG96& modem) :
			m_fileSizeInBytes(0), m_currBufferTop(0), m_currBufferBottom(0), m_modem(
					modem), m_fileHandle(-1)
	{

	}

	inline uint32_t getFileSizeInBytes()
	{
		return m_fileSizeInBytes;
	}

	inline int8_t getFileHandle()
	{
		return m_fileHandle;
	}
	virtual int available();
	virtual int read();
	virtual size_t readBytes(uint8_t *buffer, size_t length);
	virtual size_t write(uint8_t);
	virtual int peek();
	virtual int seek(uint32_t offset);
	virtual void flush();

	void startFileStream(const char* fileName, uint32_t sizeBytes);
	void stopFileStream();
	void spaceinfo();

};

#endif
