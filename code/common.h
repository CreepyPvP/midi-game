#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#ifdef WINDOWS
#include "Process.h"
// #include "processthreadsapi.h"
#endif

#ifdef UNIX
#include<unistd.h>
#endif

#include <stdlib.h>

#define lengthof(x) (sizeof(x) / sizeof(x[0]))

typedef uint32_t u32;
typedef int64_t i64;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef float f32;
typedef double f64;

#define ZED_NET_IMPLEMENTATION
#include "external/zed_net.h"
#include "external/nanomodbus.c"

void SleepMilliseconds(i32 milliseconds)
{
#ifdef WINDOWS
    Sleep(milliseconds);
#endif

#ifdef UNIX
    usleep(milliseconds * 1000);
#endif
}

char* ReadFile(const char* file)
{
    FILE* fptr = fopen(file, "rb");
    if (fptr == NULL) 
    {
        printf("Failed to read file: %s\n", file);
        return NULL;
    }
    fseek(fptr, 0, SEEK_END);
    i32 len = ftell(fptr);
    char* buf = NULL;
    buf = (char*) malloc(len + 1);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, len, 1, fptr);
    buf[len] = 0;
    fclose(fptr);
    return buf;
}

struct CommState
{
    nmbs_t nmbs;

    zed_net_socket_t socket;
    bool connected;
};

CommState comm_state;

//
//  return -1 for error.
//  return > 0 for amount of data read / written
//
i32 ReadTCP(u8 *buffer, u16 count, i32 timeout, void* arg)
{
    zed_net_socket_t *socket = (zed_net_socket_t *) arg;
    return zed_net_tcp_socket_receive(socket, buffer, count);
}

i32 WriteTCP(const u8* buffer, u16 count, i32 timeout, void* arg)
{
    zed_net_socket_t *socket = (zed_net_socket_t *) arg;

    if (zed_net_tcp_socket_send(socket, buffer, count))
    {
        return -1;
    }

    return count;
}

void InitializeModbus()
{
    comm_state = {};

    zed_net_init();

    nmbs_platform_conf platform_conf;
    nmbs_platform_conf_create(&platform_conf);
    platform_conf.transport = NMBS_TRANSPORT_TCP;
    platform_conf.read = ReadTCP;
    platform_conf.write = WriteTCP;
    platform_conf.arg = &comm_state.socket; 

    nmbs_error err = nmbs_client_create(&comm_state.nmbs, &platform_conf);

    if (err != NMBS_ERROR_NONE) {
	printf("Error creating modbus client\n");
    }
}

void ShutdownModbus()
{
    zed_net_socket_close(&comm_state.socket);
    zed_net_shutdown();
}

bool ConnectToControl(u32 port)
{
    if (comm_state.connected)
    {
	return true;
    }

    const char *host = "192.168.178.2";

    zed_net_tcp_socket_open(&comm_state.socket, 0, 0, 0);

    zed_net_address_t address;
    if (zed_net_get_address(&address, host, port)) 
    {
        printf("Error: %s\n", zed_net_get_error());
        zed_net_socket_close(&comm_state.socket);
        return false;
    }
        
    if (zed_net_tcp_connect(&comm_state.socket, address)) 
    {
        printf("Failed to connect to control\n");
        zed_net_socket_close(&comm_state.socket);
        return false;
    }

    printf("Connected to control\n");

    comm_state.connected = true;
    return true;
}
        
void ReadControlRegisters(u32 index, u32 count, u16 *result)
{
    nmbs_error err = nmbs_read_input_registers(&comm_state.nmbs, index, count, result);
    if (err != NMBS_ERROR_NONE) 
    {
	comm_state.connected = false;
	printf("Disconnected from control. Reconnecting...\n");
	memset(result, 0, sizeof(u16) * count);
    }
}

void WriteControlRegister(u32 index, u16 value)
{
    nmbs_write_single_register(&comm_state.nmbs, index, value);
}
