#include "udp_data_logger.h"
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/ndk/inc/netmain.h>
#include <stddef.h>
#include <math.h>

// Client IP address. 
// This variable is set in the console task
static const char* _clientIpAddress = "192.168.1.2";

// This port number is used for both the client and server. 
// This variable is set in the console task
static const Uint16 _portNumber = 52000;

// Number of lost datagrams during one logging session
static Uint32 _nbrOfLostDatagrams = 0;

// The device is a server which sends UDP data to the client
static SOCKET _serverSocket = INVALID_SOCKET;

// Struct containing the client IP address settings
static struct sockaddr_in _clientAddr;

// Struct containing the server (local) IP address settings
static struct sockaddr_in _serverAddr;

// Size in bytes of the sockaddr_in struct
static const Int32 SOCKADDR_SIZE = (Int32)sizeof(_clientAddr);

// Is true if the UDP socket is initialized and false otherwise
static int _initUdp = FALSE;

#pragma pack(push,1)  // Remove extra padding
typedef struct
{
    float cnt;  // Simulation counter
    float una;  // Phase A voltage
    float unb;  // Phase B voltage
    float unc;  // Phase C voltage
    float uab;  // Phase to phase AB voltage
    float ubc;  // Phase to phase BC voltage
    float uca;  // Phase to phase CA voltage
} udpMsg;
#pragma pack(pop)

// Struct containing the UDP data
static udpMsg _msg;

// Size in bytes of the udpMsg struct
static const Int32 MSG_SIZE = (Int32)sizeof(_msg);

/* Description: Update the data that shall be sent with UDP. */
static void udpUpdate();

/* Description: Initialize the UDP socket and data. */
static Int32 udpInit();

//----------------------------------------------------------------------------
void udpUpdate()
{
    // Just as an example, generate simulation data which is three phase voltage waveforms
    static const float M_PI = 3.1415926535897932384626433832795f;
    static const float DELTA_T = 0.1f;  // 100ms
    static const float U_NET = 230.0f;  // 230V
    static const float F_NET = 0.1f;  // 0,1Hz
    static const float M_2PI = 2.0f * M_PI;
    static const float TWOPIDIV3 = (2.0f / 3.0f) * M_PI;
    static const float FOURPIDIV3 = (4.0f / 3.0f) * M_PI;
    static const float TWOPI_DELTA_T = 2.0f * M_PI * DELTA_T;
    static Uint32 PERIOD_COUNT = (Uint32)((1.0f / (F_NET * DELTA_T)) + 0.5f);
    static float _simulCnt = 0.0f;
    static Uint32 _tCnt = 0;

    // update simulation time
    _simulCnt += DELTA_T;

    // reference angle for the three phase sine waves
    float phi_net = TWOPI_DELTA_T * F_NET * (float)_tCnt;

    // wrap the counter at every end of the period
    if ((++_tCnt) >= PERIOD_COUNT)
    {
        _tCnt = 0;
    }

    // phase a angle
    float phase_a_angle = phi_net;

    // limit phase a angle between 0 and 2*PI
    if (phase_a_angle > M_2PI)
    {
        phase_a_angle -= M_2PI;
    }
    else if (phase_a_angle < 0.0f)
    {
        phase_a_angle += M_2PI;
    }
    else
    {
        // no change in the value of _phase_a_angle
    }

    // phase b angle is shifted 2*PI/3 radians (120 degrees)
    float phase_b_angle = phi_net - TWOPIDIV3;

    // limit phase b angle between 0 and 2*PI
    if (phase_b_angle > M_2PI)
    {
        phase_b_angle -= M_2PI;
    }
    else if (phase_b_angle < 0.0f)
    {
        phase_b_angle += M_2PI;
    }
    else
    {
        // no change in the value of _phase_b_angle
    }

    // phase b angle is shifted 4*PI/3 radians (240 degrees)
    float phase_c_angle = phi_net - FOURPIDIV3;

    // keep phase c angle between 0 and 2*PI
    if (phase_c_angle > M_2PI)
    {
        phase_c_angle -= M_2PI;
    }
    else if (phase_c_angle < 0.0f)
    {
        phase_c_angle += M_2PI;
    }
    else
    {
        // no change in the value of _phase_c_angle
    }

    // generation of the three phase voltage sine waves
    float u_na = U_NET * sinf(phase_a_angle);
    float u_nb = U_NET * sinf(phase_b_angle);
    float u_nc = U_NET * sinf(phase_c_angle);

    // line (phase-to-phase) voltages
    float u_ab = u_na - u_nb;
    float u_bc = u_nb - u_nc;
    float u_ca = u_nc - u_na;

    // Copy the simulated data to the message struct
    _msg.cnt = _simulCnt;
    _msg.una = u_na;
    _msg.unb = u_nb;
    _msg.unc = u_nc;
    _msg.uab = u_ab;
    _msg.ubc = u_bc;
    _msg.uca = u_ca;
}

//----------------------------------------------------------------------------
Int32 udpInit()
{
    // Initialize the struct containing the UDP data
    memset(&_msg, 0, (size_t)MSG_SIZE);

    // Open a file descriptor on the current task which is needed before
    // the socket APIs can be used
    if (fdOpenSession(TaskSelf()) == 0)
    {
        UART_printf("Error: Opening the file descriptor failed\r");
        return -1;
    }

    // Create socket
    if (_serverSocket == INVALID_SOCKET)
    {
        _serverSocket = NDK_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (_serverSocket == INVALID_SOCKET)
    {
        UART_printf("Error: Socket creation failed. Error code: %d\r", fdError());
        return -1;
    }

    // Server (local) IP address settings
    memset(&_serverAddr, 0, (size_t)SOCKADDR_SIZE);
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = NDK_htons(_portNumber);
    _serverAddr.sin_addr.s_addr = NDK_htonl(INADDR_ANY);

    // Bind socket to the server IP address setting
    if (NDK_bind(_serverSocket, (struct sockaddr*)&_serverAddr, SOCKADDR_SIZE) == -1)
    {
        UART_printf("Error: Socket binding failed. Error code: %d\r", fdError());
        return -1;
    }

    // Client IP address settings
    memset(&_clientAddr, 0, (size_t)SOCKADDR_SIZE);
    _clientAddr.sin_family = AF_INET;
    _clientAddr.sin_port = NDK_htons(_portNumber);
    if (inet_aton((const char*)_clientIpAddress, &_clientAddr.sin_addr) == 0)
    {
        UART_printf("Error: %s client IP address is invalid\r", (const char*)_clientIpAddress);
        return -1;
    }

    _initUdp = TRUE;
    UART_printf("UDP data logger enabled\r");
    return 0;
}

//----------------------------------------------------------------------------
void udpSend()
{
    if (_initUdp == FALSE)
    {
        // Initialize UDP socket
        if (udpInit() == -1)
        {
            UART_printf("\nUDP logger not correctly initialized\n");
        }
    }
    else
    {
        // Update data
        udpUpdate();

        // Send data
        Int32 nbrOfBytesSent = NDK_sendto(_serverSocket, &_msg, MSG_SIZE, MSG_DONTROUTE, (struct sockaddr*)&_clientAddr, SOCKADDR_SIZE);
        if (nbrOfBytesSent == -1)
        {
            UART_printf("Error: Sending UDP data failed. Error code: %d\r", fdError());
        }
        else
        {
            if (nbrOfBytesSent != MSG_SIZE)
            {
                ++_nbrOfLostDatagrams;
            }
        }
    }
}

//----------------------------------------------------------------------------
UInt32 getLostDatagrams()
{
    return _nbrOfLostDatagrams;
}
