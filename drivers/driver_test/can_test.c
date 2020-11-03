/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        can_test.c
 *
 * @brief       The test file for can.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <os_task.h>
#include <os_device.h>
#include <os_sem.h>
#include <os_errno.h>
#include <os_clock.h>
#include <os_assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <shell.h>
#include <drivers.h>
#include <can/can.h>

#define CAN_DATA_DUMP

static struct os_semaphore rx_sem;

static os_device_t *can_dev;

static int rx_count = 0;
static int tx_count = 0;

static uint16_t tx_crc = 0;
static uint16_t rx_crc = 0;

static os_err_t can_rx_call(os_device_t *dev, os_size_t size)
{
    os_sem_post(&rx_sem);
    rx_count++;

    return OS_EOK;
}

static void can_rx_task(void *parameter)
{
    struct os_can_msg rxmsg = {0};

    os_device_set_rx_indicate(can_dev, can_rx_call);
    
    while (1)
    {
        rxmsg.hdr = -1;
        os_sem_wait(&rx_sem, OS_IPC_WAITING_FOREVER);
        if (os_device_read(can_dev, 0, &rxmsg, sizeof(rxmsg)) > 0)
        {
            rx_crc = crc16(rx_crc, rxmsg.data, 8);
#ifdef CAN_DATA_DUMP
            os_kprintf("recv(%d) id:%x ", rx_count, rxmsg.id);
            hex_dump(rxmsg.data, 8);
#endif
        }
    }
}

int can_start(int argc, char *argv[])
{
    os_err_t    res;
    os_task_t  *can_task;
    os_uint32_t baud = CAN500kBaud;
    os_uint8_t  mode = OS_CAN_MODE_NORMAL;
    char       *can_name;

    if (argc < 2)
    {
        os_kprintf("usage: can_start <dev> [baud] [mode]\r\n");
        os_kprintf("       can_start can1  [500000] [mode]\r\n");
        os_kprintf("       can_start can1  500000 0(normal)\r\n");
        os_kprintf("       can_start can1  500000 1(listen)\r\n");
        os_kprintf("       can_start can1  500000 2(loopback)\r\n");
        os_kprintf("       can_start can1  500000 3(listen loopback)\r\n");
        return -1;
    }

    can_name = argv[1];

    if (argc > 2)
    {
        baud = strtol(argv[2], OS_NULL, 0);
    }

    if (argc > 3)
    {
        mode = strtol(argv[3], OS_NULL, 0);
    }

    os_kprintf("can:%s, baud:%d.\r\n", can_name, baud);
    
    can_dev = os_device_find(can_name);
    if (!can_dev)
    {
        os_kprintf("find %s failed!\n", can_name);
        return OS_ERROR;
    }
    os_sem_init(&rx_sem, "rx_sem", 0, OS_IPC_FLAG_FIFO);

    res = os_device_open(can_dev, OS_DEVICE_OFLAG_RDWR);
    OS_ASSERT(res == OS_EOK);

    res = os_device_control(can_dev, OS_CAN_CMD_SET_BAUD, (void *)baud);
    res = os_device_control(can_dev, OS_CAN_CMD_SET_MODE, (void *)mode);


    can_task = os_task_create("can_rx", can_rx_task, OS_NULL, 1024, 25, 10);
    if (can_task != OS_NULL)
    {
        os_task_startup(can_task);
    }
    else
    {
        os_kprintf("create can_rx task failed!\n");
    }
    return res;
}
SH_CMD_EXPORT(can_start, can_start, "can device sample");

static os_err_t can_tx_done(os_device_t *uart, void *buffer)
{
    //os_kprintf("<%d>tx done\r\n", os_tick_get());
    tx_count++;
    return 0;
}

void can_send(int argc, char *argv[])
{
    struct os_can_msg msg = {0};
    os_size_t size;

    int loops = 1;

    if (argc == 2)
    {
        loops = strtol(argv[1], OS_NULL, 0);
    }
    
    msg.id      = 0x68;
    msg.ide     = OS_CAN_STDID;
    msg.rtr     = OS_CAN_DTR;
    msg.len     = 8;

    rx_count = 0;
    tx_crc = 0;
    rx_crc = 0;

    tx_count = 0;

    os_device_set_tx_complete(can_dev, can_tx_done);

    while (loops--)
    {
        msg.data[0] = rand();
        msg.data[1] = rand();
        msg.data[2] = rand();
        msg.data[3] = rand();
        msg.data[4] = rand();
        msg.data[5] = rand();
        msg.data[6] = rand();
        msg.data[7] = rand();
    
        tx_crc = crc16(tx_crc, msg.data, 8);

#ifdef CAN_DATA_DUMP
        os_kprintf("send id:%x ", msg.id);
        hex_dump(msg.data, 8);
#endif
        size = os_device_write(can_dev, 0, &msg, sizeof(msg));
        if (size == 0)
        {
            os_kprintf("can dev write data failed!\n");
        }
    }

    os_task_msleep(3000);
    os_kprintf("rx_count: %d\r\n", rx_count);
    os_kprintf("tx_count: %d\r\n", tx_count);
    os_kprintf("    %s tx_crc:%04x, rx_crc:%04x\r\n", (tx_crc == rx_crc) ? "success" : "failed", tx_crc, rx_crc);
}
SH_CMD_EXPORT(can_send, can_send, "send can data");
