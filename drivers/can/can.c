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
 * @file        can.c
 *
 * @brief       This file provides functions for registering can device.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <os_hw.h>
#include <os_task.h>
#include <os_device.h>
#include <os_errno.h>
#include <can/can.h>
#include <os_assert.h>
#include <os_mutex.h>
#include <os_list.h>
#include <os_memory.h>
#include <os_util.h>
#include <string.h>

/**
 ***********************************************************************************************************************
 * @def         CAN_LOCK(can)
 *
 * @brief       lock mutex
 *
 * @param       can             pointer to os_can_device
 ***********************************************************************************************************************
 */
#define CAN_LOCK(can)   os_mutex_lock(&(can->lock), OS_IPC_WAITING_FOREVER)
#define CAN_UNLOCK(can) os_mutex_unlock(&(can->lock))

static os_err_t os_can_init(struct os_device *dev)
{
    os_err_t              result = OS_EOK;
    struct os_can_device *can;

    OS_ASSERT(dev != OS_NULL);
    can = (struct os_can_device *)dev;

    /* initialize rx/tx */
    can->can_rx = OS_NULL;
    can->can_tx = OS_NULL;

    /* apply configuration */
    if (can->ops->configure)
        result = can->ops->configure(can, &can->config);
    else
        result = OS_ENOSYS;

    return result;
}

/*
 * can interrupt routines
 */
static int _can_int_rx(struct os_can_device *can, struct os_can_msg *data, int msgs)
{
    int size;
    os_base_t level;
    struct os_can_rx_fifo  *rx_fifo;
    struct os_can_msg_list *listmsg;
    
    OS_ASSERT(can != OS_NULL);
    rx_fifo = can->can_rx;
    OS_ASSERT(rx_fifo != OS_NULL);

    for (size = 0; size < msgs; size += sizeof(struct os_can_msg))
    {
        level = os_hw_interrupt_disable();
        if (os_list_empty(&rx_fifo->datalist))
        {
            os_hw_interrupt_enable(level);
            break;
        }
        listmsg = os_list_first_entry(&rx_fifo->datalist, struct os_can_msg_list, list);
        os_list_del(&listmsg->list);
        rx_fifo->rx_available--;
        os_hw_interrupt_enable(level);

        *data++ = listmsg->data;

        level = os_hw_interrupt_disable();
        if (os_list_empty(&rx_fifo->freelist))
            can->ops->start_recv(can, &listmsg->data);
        os_list_add_tail(&rx_fifo->freelist, &listmsg->list);
        os_hw_interrupt_enable(level);
    }

    return size;
}

static int _can_int_tx(struct os_can_device *can, const struct os_can_msg *data, int msgs)
{
    os_base_t level;
    int ret;
    int size = msgs;
    
    struct os_can_tx_fifo       *tx_fifo;
    struct os_can_sndbxinx_list *tx_tosnd;

    OS_ASSERT(can != OS_NULL);
    tx_fifo = can->can_tx;
    OS_ASSERT(tx_fifo != OS_NULL);

    for (size = 0; size < msgs; size += sizeof(struct os_can_msg))
    {
        ret = os_sem_wait(&tx_fifo->sem, OS_IPC_WAITING_FOREVER);
        if (ret != OS_EOK)
        {
            os_kprintf("can device has been closed!");
            return 0;
        }

        level    = os_hw_interrupt_disable();
        tx_tosnd = os_list_first_entry(&tx_fifo->freelist, struct os_can_sndbxinx_list, list);
        OS_ASSERT(tx_tosnd != OS_NULL);

        os_list_del(&tx_tosnd->list);
        os_hw_interrupt_enable(level);

        tx_tosnd->data = *data;
        
        level = os_hw_interrupt_disable();

        if (os_list_empty(&tx_fifo->datalist))
        {
            ret = can->ops->start_send(can, &tx_tosnd->data);
            if (ret != 0)
            {
                os_list_add_tail(&tx_fifo->freelist, &tx_tosnd->list);
                os_sem_post(&tx_fifo->sem);
                os_hw_interrupt_enable(level);
                os_kprintf("%s send failed %d.\r\n", can->parent.parent.name, ret);
                break;
            }
        }
        
        os_list_add_tail(&tx_fifo->datalist, &tx_tosnd->list);
        
        os_hw_interrupt_enable(level);

        can->status.sndchange = 1;
    }

    return size;
}

static os_err_t os_can_open(struct os_device *dev, os_uint16_t oflag)
{
    struct os_can_device *can;
    char tmpname[16];
    
    OS_ASSERT(dev != OS_NULL);
    can = (struct os_can_device *)dev;

    CAN_LOCK(can);

    dev->open_flag = oflag & 0xff;
    
    if (can->can_rx == OS_NULL)
    {
        int i = 0;
        struct os_can_rx_fifo *rx_fifo;

        rx_fifo = os_calloc(1, sizeof(struct os_can_rx_fifo) + can->config.msgboxsz * sizeof(struct os_can_msg_list));
        OS_ASSERT(rx_fifo != OS_NULL);

        rx_fifo->buffer = (struct os_can_msg_list *)(rx_fifo + 1);
        os_list_init(&rx_fifo->freelist);
        os_list_init(&rx_fifo->datalist);
        for (i = 0; i < can->config.msgboxsz; i++)
        {
            os_list_add_tail(&rx_fifo->freelist, &rx_fifo->buffer[i].list);
        }
        can->can_rx = rx_fifo;

        can->ops->start_recv(can, &rx_fifo->buffer[0].data);
    }

    if (can->can_tx == OS_NULL)
    {
        int                    i = 0;
        struct os_can_tx_fifo *tx_fifo;

        tx_fifo = os_calloc(1, sizeof(struct os_can_tx_fifo) + can->config.sndboxnumber * sizeof(struct os_can_sndbxinx_list));
        OS_ASSERT(tx_fifo != OS_NULL);

        tx_fifo->buffer = (struct os_can_sndbxinx_list *)(tx_fifo + 1);
        os_list_init(&tx_fifo->freelist);
        os_list_init(&tx_fifo->datalist);
        for (i = 0; i < can->config.sndboxnumber; i++)
        {
            os_list_add_tail(&tx_fifo->freelist, &tx_fifo->buffer[i].list);
            tx_fifo->buffer[i].result = OS_CAN_SND_RESULT_OK;
        }

        os_sem_init(&(tx_fifo->sem), tmpname, can->config.sndboxnumber, OS_IPC_FLAG_FIFO);
        can->can_tx = tx_fifo;
    }
    
    CAN_UNLOCK(can);

    return OS_EOK;
}

static os_err_t os_can_close(struct os_device *dev)
{
    struct os_can_device *can;

    OS_ASSERT(dev != OS_NULL);
    can = (struct os_can_device *)dev;

    CAN_LOCK(can);

    /* this device has more reference count */
    if (dev->ref_count > 1)
    {
        CAN_UNLOCK(can);
        return OS_EOK;
    }

    if (can->timerinitflag)
    {
        can->timerinitflag = 0;

        os_timer_stop(&can->timer);
    }

    can->status_indicate.ind  = OS_NULL;
    can->status_indicate.args = OS_NULL;

    if (can->ops->stop_recv)
        can->ops->stop_recv(can);

    if (can->ops->stop_send)
        can->ops->stop_send(can);

    OS_ASSERT(can->can_rx != OS_NULL);
    os_free(can->can_rx);
    can->can_rx = OS_NULL;

    OS_ASSERT(can->can_tx != OS_NULL);
    os_sem_deinit(&can->can_tx->sem);
    os_free(can->can_tx);
    can->can_tx = OS_NULL;

    CAN_UNLOCK(can);

    return OS_EOK;
}

static os_size_t os_can_read(struct os_device *dev, os_off_t pos, void *buffer, os_size_t size)
{
    OS_ASSERT(dev != OS_NULL);
    
    if (size == 0)
    {
        return 0;
    }

    return _can_int_rx((struct os_can_device *)dev, buffer, size);
}

static os_size_t os_can_write(struct os_device *dev, os_off_t pos, const void *buffer, os_size_t size)
{
    OS_ASSERT(dev != OS_NULL);
    
    if (size == 0)
    {
        return 0;
    }

    return _can_int_tx((struct os_can_device *)dev, buffer, size);
}

static os_err_t os_can_control(struct os_device *dev, int cmd, void *args)
{
    struct os_can_device *can;
    os_err_t              res;

    res = OS_EOK;
    OS_ASSERT(dev != OS_NULL);
    can = (struct os_can_device *)dev;

    switch (cmd)
    {
    case OS_DEVICE_CTRL_SUSPEND:
        /* suspend device */
        dev->flag |= OS_DEVICE_FLAG_SUSPENDED;
        break;

    case OS_DEVICE_CTRL_RESUME:
        /* resume device */
        dev->flag &= ~OS_DEVICE_FLAG_SUSPENDED;
        break;

    case OS_DEVICE_CTRL_CONFIG:
        /* configure device */
        res = can->ops->configure(can, (struct can_configure *)args);
        break;

    case OS_CAN_CMD_SET_PRIV:
        /* configure device */
        if ((os_uint32_t)args != can->config.privmode)
        {
            int                    i;
            os_base_t              level;
            struct os_can_tx_fifo *tx_fifo;

            res = can->ops->control(can, cmd, args);
            if (res != OS_EOK)
                return res;
            tx_fifo = (struct os_can_tx_fifo *)can->can_tx;
            if (can->config.privmode)
            {
                for (i = 0; i < can->config.sndboxnumber; i++)
                {
                    level = os_hw_interrupt_disable();
                    if (os_list_empty(&tx_fifo->buffer[i].list))
                    {
                        os_sem_post(&(tx_fifo->sem));
                    }
                    else
                    {
                        os_list_del(&tx_fifo->buffer[i].list);
                    }
                    os_hw_interrupt_enable(level);
                }
            }
            else
            {
                for (i = 0; i < can->config.sndboxnumber; i++)
                {
                    level = os_hw_interrupt_disable();
                    if (tx_fifo->buffer[i].result == OS_CAN_SND_RESULT_OK)
                    {
                        os_list_add_tail(&tx_fifo->freelist, &tx_fifo->buffer[i].list);
                    }
                    os_hw_interrupt_enable(level);
                }
            }
        }
        break;

    case OS_CAN_CMD_SET_STATUS_IND:
        can->status_indicate.ind  = ((os_can_status_ind_type_t)args)->ind;
        can->status_indicate.args = ((os_can_status_ind_type_t)args)->args;
        break;

    case OS_CAN_CMD_SET_FILTER:
        res = can->ops->control(can, cmd, args);
        if (res != OS_EOK)
        {
            return res;
        }
#ifdef OS_CAN_USING_HDR
        
        if (can->hdr == OS_NULL)
            return -1;
        
        struct os_can_filter_config *pfilter;
        struct os_can_filter_item   *pitem;
        os_uint32_t    count;
        os_base_t      level;

        pfilter = (struct os_can_filter_config *)args;
        OS_ASSERT(pfilter);
        count = pfilter->count;
        pitem = pfilter->items;
        if (pfilter->actived)
        {
            while (count)
            {
                if (pitem->hdr >= can->config.maxhdr || pitem->hdr < 0)
                {
                    count--;
                    pitem++;
                    continue;
                }

                level = os_hw_interrupt_disable();
                if (!can->hdr[pitem->hdr].connected)
                {
                    os_hw_interrupt_enable(level);
                    memcpy(&can->hdr[pitem->hdr].filter, pitem, sizeof(struct os_can_filter_item));
                    level                          = os_hw_interrupt_disable();
                    can->hdr[pitem->hdr].connected = 1;
                    can->hdr[pitem->hdr].msgs      = 0;
                    os_list_init(&can->hdr[pitem->hdr].list);
                }
                os_hw_interrupt_enable(level);

                count--;
                pitem++;
            }
        }
        else
        {
            while (count)
            {
                if (pitem->hdr >= can->config.maxhdr || pitem->hdr < 0)
                {
                    count--;
                    pitem++;
                    continue;
                }
                level = os_hw_interrupt_disable();

                if (can->hdr[pitem->hdr].connected)
                {
                    can->hdr[pitem->hdr].connected = 0;
                    can->hdr[pitem->hdr].msgs      = 0;
                    if (!os_list_empty(&can->hdr[pitem->hdr].list))
                    {
                        os_list_del(can->hdr[pitem->hdr].list.next);
                    }
                    os_hw_interrupt_enable(level);
                    memset(&can->hdr[pitem->hdr].filter, 0, sizeof(struct os_can_filter_item));
                }
                else
                {
                    os_hw_interrupt_enable(level);
                }
                count--;
                pitem++;
            }
        }
#endif
        break;

    default:
        /* control device */
        if (can->ops->control != OS_NULL)
        {
            res = can->ops->control(can, cmd, args);
        }
        else
        {
            res = OS_ENOSYS;
        }
        break;
    }

    return res;
}

#ifdef OS_USING_DEVICE_OPS
const static struct os_device_ops can_device_ops =
{
    os_can_init,
    os_can_open,
    os_can_close,
    os_can_read,
    os_can_write,
    os_can_control
};
#endif

/**
 ***********************************************************************************************************************
 * @brief           can register
 *
 * @param[in]       can             pointer to struct os_can_device.
 * @param[in]       name            can device's name.
 * @param[in]       ops             Operation function set.
 * @param[in]       data            can device private data.
 *
 * @return          Return register status.
 * @retval          OS_EOK          register success.
 * @retval          Others          register failed.
 ***********************************************************************************************************************
 */
os_err_t os_hw_can_register(struct os_can_device *can, const char *name, const struct os_can_ops *ops, void *data)
{
    struct os_device *device;
    OS_ASSERT(can != OS_NULL);

    device = &(can->parent);

    device->type        = OS_DEVICE_TYPE_CAN;
    device->rx_indicate = OS_NULL;
    device->tx_complete = OS_NULL;
    can->can_rx = OS_NULL;
    can->can_tx = OS_NULL;
    os_mutex_init(&(can->lock), "can", OS_IPC_FLAG_PRIO, OS_FALSE);
#ifdef OS_CAN_USING_BUS_HOOK
    can->bus_hook = OS_NULL;
#endif /*OS_CAN_USING_BUS_HOOK*/

#ifdef OS_USING_DEVICE_OPS
    device->ops = &can_device_ops;
#else
    device->init    = os_can_init;
    device->open    = os_can_open;
    device->close   = os_can_close;
    device->read    = os_can_read;
    device->write   = os_can_write;
    device->control = os_can_control;
#endif
    can->ops        = ops;

    can->status_indicate.ind  = OS_NULL;
    can->status_indicate.args = OS_NULL;
    memset(&can->status, 0, sizeof(can->status));

    device->user_data  = data;

    return os_device_register(device, name, OS_DEVICE_FLAG_RDWR);
}

/**
 ***********************************************************************************************************************
 * @brief           ISR for can interrupt.
 *
 * @param[in]       can             pointer to os_can_device.
 * @param[in]       event           can event.
 *
 * @return          none
 ***********************************************************************************************************************
 */
void os_hw_can_isr_rxdone(struct os_can_device *can)
{
    struct os_can_rx_fifo * rx_fifo;
    struct os_can_msg_list *listmsg = OS_NULL;

    rx_fifo = (struct os_can_rx_fifo *)can->can_rx;
    OS_ASSERT(rx_fifo != OS_NULL);

    can->status.rcvpkg++;
    can->status.rcvchange = 1;

    listmsg = os_list_first_entry_or_null(&rx_fifo->freelist, struct os_can_msg_list, list);

    OS_ASSERT(listmsg != OS_NULL);

    os_list_del(&listmsg->list);

    os_list_add_tail(&rx_fifo->datalist, &listmsg->list);
    
    rx_fifo->rx_available++;

    if (can->parent.rx_indicate != OS_NULL)
    {
        can->parent.rx_indicate(&can->parent, rx_fifo->rx_available);
    }

    listmsg = os_list_first_entry_or_null(&rx_fifo->freelist, struct os_can_msg_list, list);
    if (listmsg != OS_NULL)
        can->ops->start_recv(can, &listmsg->data);
}

void os_hw_can_isr_txdone(struct os_can_device *can, int event)
{
    struct os_can_tx_fifo *tx_fifo;
    struct os_can_sndbxinx_list *tx_tosnd;

    tx_fifo = (struct os_can_tx_fifo *)can->can_tx;
    OS_ASSERT(tx_fifo != OS_NULL);

    tx_tosnd = os_list_first_entry(&tx_fifo->datalist, struct os_can_sndbxinx_list, list);

    if ((event & 0xff) == OS_CAN_EVENT_TX_DONE)
    {        
        tx_tosnd->result = OS_CAN_SND_RESULT_OK;
        can->status.sndpkg++;
    }
    else
    {
        tx_tosnd->result = OS_CAN_SND_RESULT_ERR;
        can->status.dropedsndpkg++;
    }

    os_list_del(&tx_tosnd->list);

    os_list_add_tail(&tx_fifo->freelist, &tx_tosnd->list);

    os_sem_post(&tx_fifo->sem);

    if (os_list_empty(&tx_fifo->datalist))
    {        
        if (can->parent.tx_complete != OS_NULL)
        {
            can->parent.tx_complete(&can->parent, OS_NULL);
        }
    }

    tx_tosnd = os_list_first_entry_or_null(&tx_fifo->datalist, struct os_can_sndbxinx_list, list);
    if (tx_tosnd != OS_NULL)
        can->ops->start_send(can, &tx_tosnd->data);
}

