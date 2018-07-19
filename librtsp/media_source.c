/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/

#include "media_source.h"
#include "sdp.h"
#include <libdict.h>
#include <libmacro.h>
#include <libatomic.h>
#include <strings.h>

extern const struct media_source_handle live_media_source;
extern const struct media_source_handle h264_media_source;

#define REGISTER_MEDIA_SOURCE(x)                                               \
    {                                                                          \
        extern struct media_source media_source_##x;                           \
            media_source_register(&media_source_##x);                          \
    }

static struct media_source *first_media_source = NULL;
static struct media_source **last_media_source = &first_media_source;
static int registered = 0;

static void media_source_register(struct media_source *ms)
{
    struct media_source **p = last_media_source;
    ms->next = NULL;
    while (*p || atomic_ptr_cas((void * volatile *)p, NULL, ms))
        p = &(*p)->next;
    last_media_source = &ms->next;
}

void media_source_register_all(void)
{
    if (registered)
        return;
    registered = 1;

    REGISTER_MEDIA_SOURCE(h264);
}

struct media_source *media_source_lookup(char *name)
{
    struct media_source *p;
    for (p = first_media_source; p != NULL; p = p->next) {
        if (!strcasecmp(name, p->name))
            break;
    }
    return p;
}

void *transport_session_pool_create()
{
    return (void *)dict_new();
}

void transport_session_pool_destroy(void *pool)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate((dict *)pool, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        free(val);
    }
    dict_free((dict *)pool);
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

static uint16_t g_rtp_port_base = 20000;

struct transport_session *transport_session_new(void *pool)
{
    char key[9];
    struct transport_session *s = CALLOC(1, struct transport_session);
    s->session_id = get_random_number();
    s->rtp_port = g_rtp_port_base;
    snprintf(key, sizeof(key), "%08x", s->session_id);
    dict_add((dict *)pool, key, (char *)s);
    return s;
}

void transport_session_del(void *pool, char *name)
{
    dict_del((dict *)pool, name);
}

struct transport_session *transport_session_lookup(void *pool, char *name)
{
    return (struct transport_session *)dict_get((dict *)pool, name, NULL);
}
