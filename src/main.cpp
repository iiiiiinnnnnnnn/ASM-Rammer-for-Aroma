
#include "utils/logger.h"

#include <coreinit/filesystem.h>
#include <coreinit/title.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <wups.h>
#include <wups/button_combo/api.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemButtonCombo.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include <function_patcher/function_patching.h>

#include <forward_list>
#include <optional>
#include <nsysnet/socket.h>
#include <sys/socket.h>
#include <malloc.h>

#include <nn/ac/ac_cpp.h>

#include <kernel/kernel.h>
#include <coreinit/debug.h>
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>

#include "helper.h"

WUPS_PLUGIN_NAME("ASM Rammer");
WUPS_PLUGIN_DESCRIPTION("");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Inupawn");
WUPS_PLUGIN_LICENSE("BSD");
WUPS_USE_WUT_DEVOPTAB();

#ifndef OS_THREAD_ATTR_DETACH
#define OS_THREAD_ATTR_DETACH 0x00000001
#endif

#define SOCKET_THREAD_STACK_SIZE 0x2000
#define SOCKET_THREAD_PRIORITY   16

OSThread socketThread;
uint8_t socketThreadStack[SOCKET_THREAD_STACK_SIZE] __attribute__((aligned(8)));
volatile bool socketThreadRunning = false;

int serverSock = -1;
int clientSock = -1;
sockaddr_in clientAddr = { 0 };
const uint32_t port = 1919;

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle)
{
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    uint32_t hostIpAddress = 0;
	nn::ac::GetAssignedAddress (&hostIpAddress);

    char buffer[256];
    snprintf(buffer, 256, "%u.%u.%u.%u", 
        (hostIpAddress >> 24) & 0xFF,
        (hostIpAddress >> 16) & 0xFF,
        (hostIpAddress >> 8) & 0xFF,
        (hostIpAddress & 0xFF));

    char buffer2[256];
    snprintf(buffer2, 256, "%u.%u.%u.%u", 
        (clientAddr.sin_addr.s_addr >> 24) & 0xFF,
        (clientAddr.sin_addr.s_addr >> 16) & 0xFF,
        (clientAddr.sin_addr.s_addr >> 8) & 0xFF,
        (clientAddr.sin_addr.s_addr & 0xFF));

    root.add(WUPSConfigItemStub::Create("Server IPv4: " + std::string(buffer)));
    root.add(WUPSConfigItemStub::Create("Client IPv4: " + std::string(buffer2)));
    root.add(WUPSConfigItemStub::Create("Port: "        + std::to_string(port)));

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

INITIALIZE_PLUGIN()
{
    nn::ac::Initialize();
	//nn::ac::ConnectAsync();

    FunctionPatcher_InitLibrary();
    NotificationModule_InitLibrary();

    WUPSConfigAPIOptionsV1 configOptions;
    configOptions.name = "ASM Rammer";
    WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
}

DEINITIALIZE_PLUGIN() {
    //DEBUG_FUNCTION_LINE("DEINITIALIZE_PLUGIN!");
}

void writeKernelMemory(uint32_t addr, uint32_t value) {
    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);
    KernelCopyData(OSEffectiveToPhysical(addr), OSEffectiveToPhysical((uint32_t)&value), 4);
}

int socketThreadFunc(int arg, const char** argv)
{
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        NotificationErrorf("socket() failed");
        return 0;
    }

    int optval = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_NONBLOCK, &optval, sizeof(optval)) < 0) {
        NotificationErrorf("setsockopt(SO_NONBLOCK) failed");
        socketclose(serverSock);
        return 0;
    }
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        NotificationErrorf("setsockopt(SO_REUSEADDR) failed");
        socketclose(serverSock);
        return 0;
    }
    
    uint32_t hostIpAddress = 0;
    nn::ac::GetAssignedAddress(&hostIpAddress);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = hostIpAddress;

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0 || listen(serverSock, 1) < 0) {
        NotificationErrorf("bind() or listen() failed");
        socketclose(serverSock);
        serverSock = -1;
        return 0;
    }

    while (socketThreadRunning)
    {
        socklen_t clientAddrLen = sizeof(clientAddr);
        int sock = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrLen);
        if (sock >= 0)
        {
            clientSock = sock;

            NotificationInfof("Client connected: %u.%u.%u.%u",
                (clientAddr.sin_addr.s_addr >> 24) & 0xFF,
                (clientAddr.sin_addr.s_addr >> 16) & 0xFF,
                (clientAddr.sin_addr.s_addr >> 8) & 0xFF,
                (clientAddr.sin_addr.s_addr & 0xFF));

            char buffer[256];
            while (socketThreadRunning)
            {
                int received = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                // if (received <= 0)
                //     break;

                buffer[received] = '\0';

                if (buffer[0] == 'w') {
                    uint32_t addr = 0, value = 0;
                    if (sscanf(buffer + 1, "%X %X", &addr, &value) == 2) {
                        //NotificationInfof("Writing to %08X value %08X", addr, value);
                        writeKernelMemory(addr, value);
                    }
                } else if (buffer[0] == 'r') {
                    uint32_t addr = 0;
                    if (sscanf(buffer + 1, "%X", &addr) == 1) {
                        uint32_t value = *(volatile uint32_t*)addr;
                        send(clientSock, &value, sizeof(value), 0);
                    }
                } else if (buffer[0] == 's') {
                    break;
                }
            }

            socketclose(clientSock);
            clientSock = -1;
        }

        OSSleepTicks(OSMillisecondsToTicks(200));
    }

    if (serverSock >= 0) {
        socketclose(serverSock);
        serverSock = -1;
    }
    
    NotificationInfof("Client disconnected");

    return 0;
}

ON_APPLICATION_START()
{
    initLogging();
    DEBUG_FUNCTION_LINE("ON_APPLICATION_START!");

    uint64_t titleid = OSGetTitleID();
    if (titleid != 0x00050000101dbe00ULL
        && titleid != 0x00050000101d7500ULL
        && titleid != 0x00050000101d9d00ULL)
        return;

////////////////////////// in Minecraft //////////////////////////
    
    if (!socketThreadRunning)
    {
        socketThreadRunning = true;

        if (!OSCreateThread(&socketThread, socketThreadFunc, 0, nullptr,
                        socketThreadStack, SOCKET_THREAD_STACK_SIZE,
                        SOCKET_THREAD_PRIORITY, OS_THREAD_ATTR_DETACH))
        {
            NotificationErrorf("Failed to create socket thread");
            socketThreadRunning = false;
        }
        else
        {
            OSResumeThread(&socketThread);
        }
    }
}

/**
 * Gets called when an application actually ends
 */
ON_APPLICATION_ENDS() {
    deinitLogging();
}

/**
    Gets called when an application request to exit.
**/
ON_APPLICATION_REQUESTS_EXIT() {
    //DEBUG_FUNCTION_LINE("ON_APPLICATION_REQUESTS_EXIT of example_plugin!");
}
