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
 * @file        os_mutex.h
 *
 * @brief       Header file for synchronous I/O multiplexing interface select.
 *
 * @revision
 * Date         Author          Notes
 * 2020-04-28   OneOS team      First Version
 ***********************************************************************************************************************
 */
#ifndef __VFS_SELECT_H__
#define __VFS_SELECT_H__

#ifdef OS_USING_POSIX

#include <libc_fdset.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif /* OS_USING_POSIX */

#endif /* __VFS_SELECT_H__ */

