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
 * @file        libc.h
 *
 * @brief       Header file for libc interface.
 *
 * @revision
 * Date         Author          Notes
 * 2020-04-13   OneOS team      First Version
 ***********************************************************************************************************************
 */
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stddef.h>

int libc_system_init(void);

int libc_stdio_set_console(const char* device_name, int mode);
int libc_stdio_get_console(void);
int libc_stdio_read (void *buffer, size_t size);
int libc_stdio_write(const void *buffer, size_t size);

#endif
