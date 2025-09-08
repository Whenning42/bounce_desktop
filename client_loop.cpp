#include "client_loop.h"

#include <rfb/rfbclient.h>

#include <unordered_map>

#include "third_party/status/status_or.h"

static void setup_format(rfbClient* client) {
  client->format.bitsPerPixel = 32;
  client->format.redShift = 16;
  client->format.greenShift = 8;
  client->format.blueShift = 0;
  client->format.redMax = 255;
  client->format.greenMax = 255;
  client->format.blueMax = 255;
  SetFormatAndEncodings(client);
}

static rfbCredential* unexpected_credential_error(rfbClient* client,
                                                  int credential_type) {
  ERROR("Asked for credential of type: %d", credential_type);
  exit(1);
}

void rfb_client_log(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void run_client(int32_t port, MallocFrameBufferProc on_resize,
                GotFrameBufferUpdateProc on_update, void* client_data) {
  rfbClient* client = rfbGetClient(/*bitsPerSample=*/8, /*samplesPerPixel=*/3,
                                   /*bytesPerPixel*/ 4);
  rfbClientSetClientData(client, /*tag=*/nullptr, client_data);

  // TODO: Consider handling clipboard.
  // TODO: Consider setting rfbClientLog to a logging function.
  rfbClientLog = rfb_client_log;
  client->serverHost = "localhost";
  client->serverPort = port;
  client->listenPort = port;
  client->listen6Port = port;

  client->MallocFrameBuffer = on_resize;
  client->canHandleNewFBSize = TRUE;
  client->GotFrameBufferUpdate = on_update;
  client->GetCredential = unexpected_credential_error;

  int client_argc = 1;
  char* client_argv[] = {"-encodings=raw"};
  if (!rfbInitClient(client, &client_argc, client_argv)) {
    ERROR("Failed to start the server");
  }

  while (true) {
    int r = WaitForMessage(client, 1'000);
    if (r < 0) {
      ERROR("Wait for message error: %d", r);
    }
    if (r == 0) {
      continue;
    }
    rfbBool ret = HandleRFBServerMessage(client);
    if (ret == FALSE) {
      break;
    }
  }
}
