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
 * @file        syscall_mem.c
 *
 * @brief       This file redirects the memory management interface.
 *
 * @revision
 * Date         Author          Notes
 * 2020-04-14   OneOS Team      First version.
 ***********************************************************************************************************************
 */
#include <oneos_config.h>
#include <stddef.h>
#include <os_memory.h>

#ifdef OS_USING_HEAP
void *malloc(os_size_t n)
{
    return os_malloc(n);
}

void *realloc(void *rmem, os_size_t newsize)
{
    return os_realloc(rmem, newsize);
}

void *calloc(os_size_t nelem, os_size_t elsize)
{
    return os_calloc(nelem, elsize);
}

void free(void *rmem)
{
    os_free(rmem);
}
#endif
