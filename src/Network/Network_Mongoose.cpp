#include <stdio.h>
#include "jsonxx.h"
#include "mongoose.h"
#include "../Cmdline.h"

#include "Network.h"

namespace Network
{
	static int s_done = 0;
	static int s_is_connected = 0;

	bool NewShaderToGrab = false;
	jsonxx::Object LastGrabberShader;
	float LastSendTime = 0.0f;
	float ShaderUpdateInterval = 0.3f;
  
	enum NetMode {
		NetMode_Sender,
		NetMode_Grabber
	};

	bool bNetworkEnabled = false;
	bool bNetworkLaunched = false;
	std::string ServerURL;
	std::string NetworkModeString;
	NetMode NetworkMode = NetMode::NetMode_Sender;

	void RecieveShader(size_t size, unsigned char *data);

	static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
		(void)nc;

		switch (ev) {
			case MG_EV_CONNECT: {
				int status = *((int *)ev_data);
				if (status != 0) {
					printf("-- Connection error: %d\n", status);
				}
				break;
			}
			case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
				struct http_message *hm = (struct http_message *) ev_data;
				if (hm->resp_code == 101) {
					printf("-- Connected\n");
					s_is_connected = 1;
					bNetworkLaunched = true;
				}
				else {
					printf("-- Connection failed! HTTP code %d\n", hm->resp_code);
					/* Connection will be closed after this. */
				}
				break;
			}
			case MG_EV_POLL: {
				//char msg[500];
				//strcpy(msg, "Bonjour de loin");
				//mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, strlen(msg));
				break;
			}
			case MG_EV_WEBSOCKET_FRAME: {
				struct websocket_message *wm = (struct websocket_message *) ev_data;
				printf("Grabbed message %d.\n", (int)wm->size);
				RecieveShader(wm->size, wm->data);
				// TODO: clean the buffer with mbuf_remove(); ? or maybe not needed with websocket ...
				break;
			}
			case MG_EV_CLOSE: {
				if (s_is_connected) printf("-- Disconnected\n");
				s_done = 1;
				bNetworkLaunched = false;
				break;
			}
		}
	}

	struct mg_mgr mgr;
	struct mg_connection *nc;
  
	void LoadSettings(jsonxx::Object & o, NETWORK_SETTINGS* DialogSettings)
	{
		if (o.has<jsonxx::Object>("network"))
		{
			jsonxx::Object netjson = o.get<jsonxx::Object>("network");
			if (netjson.has<jsonxx::Number>("udpateInterval"))
				ShaderUpdateInterval = netjson.get<jsonxx::Number>("udpateInterval");
		}
    	bNetworkEnabled = DialogSettings->EnableNetwork;
      ServerURL = DialogSettings->ServerURL;
	    NetworkModeString = DialogSettings->NetworkModeString;
	}

	void CommandLine(int argc, const char *argv[]) {
		if (CmdHasOption(argc, argv, "serverURL", &ServerURL)) {
			bNetworkEnabled = true;
			printf("Set server URL to %s \n", ServerURL.c_str());
		}
		if (CmdHasOption(argc, argv, "networkMode", &NetworkModeString)) {
			bNetworkEnabled = true;
			printf("Set server mode to %s \n", NetworkModeString.c_str());
		}
	}

	void OpenConnection()
	{
		if (!bNetworkEnabled) return;
		
		if (NetworkModeString == "grabber") {
			NetworkMode = NetMode_Grabber;
		}

		if (NetworkMode == NetMode_Grabber) {
			printf("Network Launching as grabber\n");
		} else {
			printf("Network Launching as sender\n");
		}

		mg_mgr_init(&mgr, NULL);

		// ws://127.0.0.1:8000
		printf("Try to connect to %s\n", ServerURL.c_str());
		nc = mg_connect_ws(&mgr, ev_handler, ServerURL.c_str(), "ws_chat", NULL);
		if (nc == NULL) {
			fprintf(stderr, "Invalid address\n");
			return;
		}
	}

	void BroadcastMessage(const char* msg) {
		if (!bNetworkLaunched) return;

		mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, msg, strlen(msg)+1);
	}

	void SendShader(float time, ShaderMessage NewMessage) {
		if (!bNetworkLaunched) return;
		if (NetworkMode != NetMode_Sender) return;

		using namespace jsonxx;

		jsonxx::Object Data;
		Data << "Code" << std::string(NewMessage.Code);
		Data << "Compile" << NewMessage.NeedRecompile;
		Data << "Caret" << NewMessage.CaretPosition;
		Data << "Anchor" << NewMessage.AnchorPosition;

		jsonxx::Object Message = Object("Data", Data);
		std::string TextJson = Message.json();
		//printf("JSON: %s\n", TextJson.c_str());
		mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, TextJson.c_str(), TextJson.length()+1);
		LastSendTime = time;
	}

	bool IsShaderNeedUpdate(float Time) {
		if (!bNetworkLaunched) return false;
		if (NetworkMode != NetMode_Sender) return false;
		return (Time - LastSendTime >= ShaderUpdateInterval);
	}

	void RecieveShader(size_t size, unsigned char *data) {
		// TODO: very very bad, we should:
		// - use json
		// - verify size
		// - non-ascii symbols ?
		// - asynchronous update ?
		data[size-1] = '\0'; // ensure we do have an end to the char string after "size" bytes
		std::string TextJson = std::string((char*)data);
		//printf("JSON: %s\n", TextJson.c_str());
		jsonxx::Object NewShader;
		bool ErrorFound = false;
		if (NewShader.parse(TextJson)) {
			if(NewShader.has<jsonxx::Object>("Data")) {
				jsonxx::Object Data = NewShader.get<jsonxx::Object>("Data");
				if (!Data.has<jsonxx::String>("Code")) ErrorFound = true;
				if (!Data.has<jsonxx::Number>("Caret")) ErrorFound = true;
				if (!Data.has<jsonxx::Number>("Anchor")) ErrorFound = true;
				if (!Data.has<jsonxx::Boolean>("Compile")) ErrorFound = true;
			} else {
				ErrorFound = true;
			}
		} else {
			ErrorFound = true;
		}
		if(ErrorFound) {
			printf("Invalid json formatting\n");
			return;
		}

		LastGrabberShader = NewShader;
		NewShaderToGrab = true;
	}

	bool IsConnected() {
		return bNetworkLaunched;
	}

	bool IsGrabber() {
		return NetworkMode == NetMode_Grabber;
	}

	bool HasNewShader() {
		if (NetworkMode != NetMode_Grabber) return false;
		return NewShaderToGrab;
	}

	bool GetNewShader(ShaderMessage& OutShader) {
		if (NetworkMode != NetMode_Grabber) return false;
		NewShaderToGrab = false;
		jsonxx::Object Data = LastGrabberShader.get<jsonxx::Object>("Data");
		OutShader.Code = Data.get<jsonxx::String>("Code");
		OutShader.CaretPosition = Data.get<jsonxx::Number>("Caret");
		OutShader.AnchorPosition = Data.get<jsonxx::Number>("Anchor");
		OutShader.NeedRecompile = Data.get<jsonxx::Boolean>("Compile");
		return true;
	}

	void Tick() {
		if (!bNetworkEnabled) return;

		// TODO: should be in another thread?
		mg_mgr_poll(&mgr, 10);
	}

	void Release() {
		if (!bNetworkLaunched) return;

		mg_mgr_free(&mgr);
	}
}