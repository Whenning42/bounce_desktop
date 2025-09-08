#include <rfb/rfbclient.h>

#include <cstdint>

#include "frame.h"

void run_client(int32_t port, MallocFrameBufferProc on_resize,
                GotFrameBufferUpdateProc on_update, void* client_data);
