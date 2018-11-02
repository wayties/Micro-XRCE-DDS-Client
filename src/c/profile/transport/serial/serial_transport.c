#include "serial_transport_internal.h"
#include <uxr/client/core/communication/serial_protocol.h>
#include <uxr/client/util/time.h>

/*******************************************************************************
 * Private function declarations.
 *******************************************************************************/
static bool send_serial_msg(void* instance, const uint8_t* buf, size_t len);
static bool recv_serial_msg(void* instance, uint8_t** buf, size_t* len, int timeout);

/*******************************************************************************
 * Private function definitions.
 *******************************************************************************/
static bool send_serial_msg(void* instance, const uint8_t* buf, size_t len)
{
    bool rv = false;
    uxrSerialTransport* transport = (uxrSerialTransport*)instance;

    uint8_t errcode;
    size_t bytes_written = uxr_write_serial_msg(&transport->serial_io,
                                                uxr_write_serial_data_platform,
                                                transport->platform,
                                                buf,
                                                len,
                                                transport->remote_addr,
                                                &errcode);
    if ((0 < bytes_written) && (bytes_written == len))
    {
        rv = true;
    }
    else
    {
        transport->comm.error_code = errcode;
    }

    return rv;
}

static bool recv_serial_msg(void* instance, uint8_t** buf, size_t* len, int timeout)
{
    bool rv = false;
    uxrSerialTransport* transport = (uxrSerialTransport*)instance;

    size_t bytes_read = 0;
    do
    {
        int64_t time_init = uxr_millis();
        uint8_t remote_addr;
        uint8_t errcode;
        bytes_read = uxr_read_serial_msg(&transport->serial_io,
                                         uxr_read_serial_data_platform,
                                         transport->platform,
                                         transport->buffer,
                                         sizeof(transport->buffer),
                                         &remote_addr,
                                         timeout,
                                         &errcode);
        if ((0 < bytes_read) && (remote_addr == transport->remote_addr))
        {
            *len = bytes_read;
            *buf = transport->buffer;
            rv = true;
        }
        else
        {
            transport->comm.error_code = errcode;
        }
        timeout -= (int)(uxr_millis() - time_init);
    }
    while ((0 == bytes_read) && (0 < timeout));

    return rv;
}

/*******************************************************************************
 * Public function definitions.
 *******************************************************************************/
bool uxr_init_serial_transport(uxrSerialTransport* transport,
                               struct uxrSerialPlatform* platfrom,
                               const int fd,
                               uint8_t remote_addr,
                               uint8_t local_addr)
{
    bool rv = false;
    if (uxr_init_serial_platform(platfrom, fd, remote_addr, local_addr))
    {
        /* Setup platform. */
        transport->platform = platfrom;

        /* Setup address. */
        transport->remote_addr = remote_addr;

        /* Init SerialIO. */
        uxr_init_serial_io(&transport->serial_io, local_addr);

        /* Setup interface. */
        transport->comm.instance = (void*)transport;
        transport->comm.send_msg = send_serial_msg;
        transport->comm.recv_msg = recv_serial_msg;
        transport->comm.error_code = 0;
        transport->comm.mtu = UXR_CONFIG_SERIAL_TRANSPORT_MTU;

        rv = true;
    }
    return rv;
}

bool uxr_close_serial_transport(uxrSerialTransport* transport)
{
    return uxr_close_serial_platform(transport->platform);
}
