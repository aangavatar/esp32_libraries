
#include "WisLTEBG96HTTP.h"
#include "SoftwareSerial.h"

#define DSerial Serial1
#define ATSerial Serial
// #define AT_TX_PIN  11
// #define AT_RX_PIN  10
// SoftwareSerial  DSerial(AT_RX_PIN, AT_TX_PIN);

//#define USE_CERT
#ifdef USE_CERT
/* Fill your certificate.pem.root CA wiht LINE ENDING */
const char pem_CA[] = {"-----BEGIN CERTIFICATE-----\n\
MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n\
yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n\
ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n\
U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n\
ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n\
aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n\
MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n\
ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n\
biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n\
U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n\
aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n\
nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n\
t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n\
SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n\
BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n\
rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n\
NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n\
BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n\
BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n\
aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n\
MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n\
p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n\
5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n\
WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n\
4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n\
hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n\
-----END CERTIFICATE-----\n"};

/* Fill your certificate.pem.crt wiht LINE ENDING */
const char pem_cert[] = {"-----BEGIN CERTIFICATE-----\n\
MIIDWjCCAkKgAwIBAgIVAPO/jH+GepSZxWRtbVnZ5WEjmF1MMA0GCSqGSIb3DQEB\n\
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n\
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xODAxMjIwNjQ2\n\
MjdaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n\
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQD0JqFo3Dg5wUj2PBAm\n\
tvCvJLw7E1Nn/J5JrEP5g7M7YCVIv2WCmAXAR5YrPct7h0rOD7h9JG4K93koDK03\n\
0u/WsqddSMedGObRqR7MSCdeAtBGKY9vrThCAQiOvl1LlmCZdyaEv5ukGUHYAGQf\n\
DCfjDvg6K4t9LTocyE2SHISv7zE1E/tHDmjZ7ups7ewE7h+35Ce9bL5tFKun2oEH\n\
4COwJFrHj8RXObXPQYFEQ1x7YbshYkjs4HXT/cKmGXGz/2kAOSPejDFdxC4Larxw\n\
Eoz6xepjjiOXOZ5w7w1M/4/LnwJl/tbGLEU+azU47n4bT3GtscbX+ddisRJ65gN2\n\
7wPHAgMBAAGjYDBeMB8GA1UdIwQYMBaAFDZV+0LrRJWxMz0PBDhwYV57/i65MB0G\n\
A1UdDgQWBBTBStT4wV7A6mkr3o6wLjyhhsmjvDAMBgNVHRMBAf8EAjAAMA4GA1Ud\n\
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAxXJMJvQBtbgv+wKisaVXtgph\n\
cuf+jtk5/N04jq7oZDkeH4+jqHmYN+mh8xS7UbnQGRl6B5ftcXIYM/Uxjb46Jhtg\n\
HnQODB6cdbb780nxT5YOQA3EcZBndhe1hBC44qICOPCp3RejGmV43EYgLzJh/wtp\n\
8yNvIEXi4t+rDP+JI7RgrgF2QlYsDYAQlr9a5JFzlLESzFanpN6PNu4/xVIO0LgI\n\
OnccrxQc5ZLcHKkze7OhjGFbmuXI9ljg/FDIXInveTV4OSQa14IfijLiZ9j5RV0o\n\
j0ansu6HbBIlwzq8ltWXfOqpAGm+YW2ukd5kKguvQHU24Tb1k2gXhHvv3mNRBA==\n\
-----END CERTIFICATE-----\n"};

/* Fill your private.pem.key wiht LINE ENDING */
const char pem_pkey[] = {"-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpAIBAAKCAQEA9CahaNw4OcFI9jwQJrbwryS8OxNTZ/yeSaxD+YOzO2AlSL9l\n\
gpgFwEeWKz3Le4dKzg+4fSRuCvd5KAytN9Lv1rKnXUjHnRjm0akezEgnXgLQRimP\n\
b604QgEIjr5dS5ZgmXcmhL+bpBlB2ABkHwwn4w74OiuLfS06HMhNkhyEr+8xNRP7\n\
Rw5o2e7qbO3sBO4ft+QnvWy+bRSrp9qBB+AjsCRax4/EVzm1z0GBRENce2G7IWJI\n\
7OB10/3Cphlxs/9pADkj3owxXcQuC2q8cBKM+sXqY44jlzmecO8NTP+Py58CZf7W\n\
xixFPms1OO5+G09xrbHG1/nXYrESeuYDdu8DxwIDAQABAoIBAFWjp0Js3JwFnF4i\n\
u1eZG9uTeyMNqFf1WGBnOICxHZFaLDnUSPsB8CJ9+cK5LkLqNecJxKDAwoi+PZU6\n\
vRezucUJbYNfIAWO0ogAVnk5ZfWn7v5tXY7Cp9UfH61SQM9etAugq7Ysr59j7KCj\n\
onrxfJw8h/1/gLIrxxKJgAwcTL6Y7LoN2oAv7qKCYtkYHVfCPeDLhjbBjuAKKP7Q\n\
t8zzVgc5Gvfe76A12oN//xaZlLI73OxMPtW5MfW718BqFcZLJzScfaY2//YiLN0F\n\
Iiccj+fZDcXPEvRaUludLZNPg2c7LO23Efn5HK66A8KJysOanfh+VBL/qdpFMCGY\n\
BXUwvOECgYEA+u084LcL0WX0TVKue+D0cBuJ4irLd3xxFOdEsVwRaeaFUQ5W9dDP\n\
PMCfgxE3a7gUPCKy7gnWTNvahudK0SqrbguTsYI4jNGpDRPkDI445KD85GAAsFBx\n\
3nbEeG1EB3DAbmIJr1Pfpi+6imXom61Po0oftBBnmyi3G1c74VQF/BsCgYEA+RZS\n\
cVA9OdY722HX2AVVQJIi3mz99QEFIX/A0AJHaNIjlFYtXUG2CQ5WanWW4lUXmyU/\n\
d7rngfACp4qUeLWBn0jiW+ylLi28ZIzzyXLE9ISgeDIA5wK+8cHHDgfmZjPz9T9K\n\
o6u6Wb9H6hJwELJFdP9AgVNPzyiou69x7Zq2OcUCgYEAzoNLdCISHt9LBgxDP/7v\n\
DgkMHNBdrlQDeVGw7ks4pbCsDf7AQQT+WiCp5yPXOddBCBECs/fyoETc/+QBZk44\n\
0pPClPf4dKTk/bdeG5s9ZaS9HXOJfZJndNDId9htgbYhGXekQuDDkzF61AZ1PxKP\n\
FXOezgImUsNal6zewl71ofsCgYAfs6SYgWLZepexqzlCaCNajX7cmjiUNyYFBnJu\n\
xut5vs0Ccxlg9h0J+AYtlHu4oKXZfBsgzOvsELT1x+BvoII32Jneoza+stl4Xqwk\n\
ZnQEvuCBi7OrsyDEOipwcbb0KQaSnxSL0i5sf7jSZLhYl6fZZSc7jnPo5L7c7E5u\n\
tDMfFQKBgQDIhY+A7xWXobtOQyaquT2IdyR3JSFfJwahLUc65nV0s582l8le+xuy\n\
ulCy291puJc1WkGr+vswMD/B2kJv61TdoxxBa2lBdwd+/RZlmuqp9iO9MHsi7oTV\n\
Y51odhXBpRYjDNieOsKEsX/xPUvUtNLJDeurI5KNr+e5dHADwvo2Jg==\n\
-----END RSA PRIVATE KEY-----\n"};
#endif
const char APN[] = "UNINET";
const char http_url[] = "https://raw.githubusercontent.com/RAKWireless/RAK811_LoRaButton/master/README.md";
unsigned int comm_pdp_index = 1;    // The range is 1 ~ 16
unsigned int comm_ssl_index = 1;  // The range if 0 ~ 5
HTTP_Body_Data_Type_t  http_type = APPLICATION_X_WWW_FORM_URLENCODED;

WisLTEBG96HTTP WisLTE(ATSerial, DSerial);

void setup(){
    DSerial.begin(115200);
    while (DSerial.read() >= 0);
    DSerial.println("This is the WisLTE Debug Serial!");
    delay(1000);
    ATSerial.begin(115200);
    while (ATSerial.read() >= 0);
    delay(1000);
    while(!WisLTE.InitModule());
    
    WisLTE.SetDevCommandEcho(false);

    char inf[64];
    if(WisLTE.GetDevInformation(inf)){
        DSerial.println(inf);
    }

    char apn_error[64];
    while (!WisLTE.InitAPN(comm_pdp_index, APN, "", "", apn_error)){
        DSerial.println(apn_error);
    }
    DSerial.println(apn_error);
#ifdef USE_CERT
    char ssl_error[128];
    while(!WisLTE.InitSSL(comm_ssl_index, pem_CA, pem_cert, pem_pkey, ssl_error)){
        DSerial.println(ssl_error);
    }
    DSerial.println(ssl_error);
#else
    char ssl_error[128];
    while(!WisLTE.InitSSL(comm_ssl_index, "", "", "", ssl_error)){
        DSerial.println(ssl_error);
    }
    DSerial.println(ssl_error);
#endif
    while(!WisLTE.SetHTTPConfigParameters(comm_pdp_index, false, false, http_type)){
        DSerial.println("\r\nConfig the HTTP Parameter Fail!");
        int e_code;
        if (WisLTE.returnErrorCode(e_code)){
            DSerial.print("\r\nERROR CODE: ");
            DSerial.println(e_code);
            DSerial.println("Please check the documentation for error details.");
            while(1);
        }
    }
    DSerial.println("\r\nConfig the HTTP Parameter Success!");

    while(!WisLTE.SetHTTPEnableSSL(comm_ssl_index)){
        DSerial.println("\r\nEnable the SSL Fail!");
        int e_code;
        if (WisLTE.returnErrorCode(e_code)){
            DSerial.print("\r\nERROR CODE: ");
            DSerial.println(e_code);
            DSerial.println("Please check the documentation for error details.");
            while(1);
        }
    }
    DSerial.println("\r\nEnable the SSL Success!");

    while(!WisLTE.HTTPURL(http_url, WRITE_MODE)){
        DSerial.println("\r\nSet the HTTP URL Fail!");
        int e_code;
        if (WisLTE.returnErrorCode(e_code)){
            DSerial.print("\r\nERROR CODE: ");
            DSerial.println(e_code);
            DSerial.println("Please check the documentation for error details.");
            while(1);
        }
    }
    DSerial.println("\r\Set the HTTP URL Success!");
}

void loop(){
    //const char recv_file[] = "http_read.txt";
    char recv_data[128];
    if(!WisLTE.HTTPGET(80)){
        DSerial.println("\r\nHTTP GET Success!");
        int e_code;
        if (WisLTE.returnErrorCode(e_code)){
            DSerial.print("\r\nERROR CODE: ");
            DSerial.println(e_code);
            DSerial.println("Please check the documentation for error details.");
            while(1);
        }
    }
    DSerial.println("\r\nHTTP GET Success!");

    // if(WisLTE.HTTPReadToFile(recv_file, 80)){
    //     DSerial.println("\r\nHTTP Read to File Success!");
    // }
    if(WisLTE.HTTPRead(recv_data, 80)){
        DSerial.println("\r\nHTTP Read Success!");
        DSerial.println("");
        DSerial.println(recv_data);
        DSerial.println("");
    }
    while(1);
}