#include "stdafx.h"
#include "safe_vector.hpp"
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXHttpClient.h>

using namespace std;

#define MT_EXPFUNC extern "C" __declspec(dllexport)
#define dbg(msg) writeLog(".\\webscoket.log", msg);

static ix::WebSocket webSocket;
static SafeVector messages;
static string last_error;

int writeLog(const char *file, const char *content) {
	if (file && content) {
		FILE *fd;
		errno_t err = fopen_s(&fd, file, "a+b");
		if (err != 0)
			return 1;
		else {
			SYSTEMTIME st;
			GetLocalTime(&st);
			fprintf(fd, "%04d.%02d.%02d %02d:%02d:%02d.%03d::%s\r\n",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, content);

			fclose(fd);
		}
	}
	return 0;
}

MT_EXPFUNC int __stdcall SetHeader(const char *key, const char * value) {
	ix::WebSocketHttpHeaders headers;
	headers[key] = value;
	webSocket.setExtraHeaders(headers);
}

MT_EXPFUNC int __stdcall Init(const char *url, int timeout, int heat_beat_period) {
	string server_url(url);

	try {
		ix::initNetSystem();

		webSocket.setUrl(server_url);

		webSocket.setHeartBeatPeriod(heat_beat_period);

		// Per message deflate connection is enabled by default. You can tweak its parameters or disable it
		webSocket.disablePerMessageDeflate();

		// Setup a callback to be fired when a message or an event (open, close, error) is received
		webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
		{
			if (msg->type == ix::WebSocketMessageType::Message)
			{
				messages.push_back(msg->str);
			}
		}
		);

		ix::WebSocketInitResult r = webSocket.connect(timeout);

		if (r.success) {
			webSocket.start();

			last_error.clear();
			return 1;
		}
		
		last_error.append(r.errorStr);
		return 0;
	}
	catch (std::exception & e) {
//		std::cerr << "websockets something wrong happened! " << std::endl;
//		std::cerr << e.what() << std::endl;
		dbg(e.what());
		last_error.append(e.what());
	}
	catch (...) {
	}


	return 0;
}

MT_EXPFUNC void __stdcall Deinit() {
	try {
		webSocket.stop();
		ix::uninitNetSystem();		
	}
	catch (std::exception & e) {
	//	std::cerr << e.what() << std::endl;
		dbg(e.what());
	}
	catch (...) {
	}

}

MT_EXPFUNC void  __stdcall WSGetLastError(char *data) {

	if (last_error.length() > 0) {
		strcpy(data, last_error.c_str());
		strcat(data, "\0");
	}
}

MT_EXPFUNC int  __stdcall httpSendPost(const char* url_, const char * input, int timeout, char *output) {
	ix::HttpResponsePtr out;
	std::string url(url_);

	ix::HttpClient httpClient;
	ix::HttpRequestArgsPtr args = httpClient.createRequest();

	args->connectTimeout = timeout;
	args->transferTimeout = timeout;

	out = httpClient.post(url, std::string(input), args);

	auto statusCode = out->statusCode;

	if (statusCode == 200) {		
		strcpy(output, out->payload.c_str());
		strcat(output, "\0");
	}
	else {
		strcpy(output, out->errorMsg.c_str());
		strcat(output, "\0");
	}

	return statusCode;
}

MT_EXPFUNC int  __stdcall GetCommand(char *data) {

	if (messages.size() > 0) {
		strcpy(data, messages.back().c_str());
		strcat(data, "\0");

		messages.pop_back();

		return 1;
	}

	return 0;
}

MT_EXPFUNC int  __stdcall SendCommand(const char *command) {
	webSocket.send(command);
	return 1;
}