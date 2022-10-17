/**
 * This file is part of the evad.
 * Copyright © Iflytek Co., Ltd.
 * All Rights Reserved.
 *
 * @author  zrmei
 * @date    2021-12-23
 *
 * @brief
 */
#ifndef EVAD_AUDIOQUEUE_H
#define EVAD_AUDIOQUEUE_H

#include <string.h>
#include <limits.h>

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sstream>
#include <thread>

#define g_list_size (20)

typedef struct BufferUnits
{
    int g_buff_size;
    // 当缓冲单元已写入字节的长度
    int g_curr_len;
    // 循环链表的当前位置
    int g_write_loop_pos;
    // 已读到的位置
    int g_read_loop_pos;
    // 临时缓冲区
    char g_temp_buff[2048];

    struct
    {
        char have_data;
        char data_buf[2048];
    } g_loop_buf[g_list_size];
} BufferUnits;

void reset_buffer(BufferUnits *g_buffer)
{
    g_buffer->g_curr_len = 0;
    g_buffer->g_write_loop_pos = 0;
    g_buffer->g_read_loop_pos = 0;
    g_buffer->g_buff_size = 2048;
    memset(g_buffer->g_loop_buf, 0, sizeof(g_buffer->g_loop_buf));
}

// 把数据写入到循环列表
int write_loop_data(BufferUnits *bt, const char *pdata, int data_len)
{
    if (bt->g_loop_buf[bt->g_write_loop_pos % g_list_size].have_data) {
        return -1;
    }

    int left = data_len;
    int used = 0;
    const char *p = pdata;
    while (left > 0) {
        if (left > bt->g_buff_size - bt->g_curr_len) {
            used = bt->g_buff_size - bt->g_curr_len;
        } else {
            used = left;
        }
        memcpy(bt->g_temp_buff + bt->g_curr_len, p, used);
        bt->g_curr_len += used;
        left -= used;
        p += used;

        if (bt->g_buff_size == bt->g_curr_len) {
            // 已满一帧数据
            bt->g_loop_buf[bt->g_write_loop_pos % g_list_size].have_data = 1;
            memcpy(bt->g_loop_buf[bt->g_write_loop_pos % g_list_size].data_buf, bt->g_temp_buff,
                   bt->g_buff_size);
            memset(bt->g_temp_buff, 0, bt->g_buff_size);

            // 游标往下移动
            bt->g_write_loop_pos = (bt->g_write_loop_pos + 1) % g_list_size;
            bt->g_curr_len = 0;
        }
    }

    return 0;
}

// 从循环列表读取数据, 返回0表示读取成功，其它表示失败
int read_loop_data(BufferUnits *bt, char *buf, int buf_len, int *actual)
{
    if (1 == bt->g_loop_buf[bt->g_read_loop_pos % g_list_size].have_data) {
        int read_size = buf_len > bt->g_buff_size ? bt->g_buff_size : buf_len;
        memcpy(buf, bt->g_loop_buf[bt->g_read_loop_pos].data_buf, read_size);
        *actual = read_size;

        bt->g_loop_buf[bt->g_read_loop_pos % g_list_size].have_data = 0;
        memset(bt->g_loop_buf[bt->g_read_loop_pos].data_buf, 0, bt->g_buff_size);

        bt->g_read_loop_pos = (bt->g_read_loop_pos + 1) % g_list_size;
        return 0;
    }

    return -1;
}

#endif    //EVAD_AUDIOQUEUE_H
