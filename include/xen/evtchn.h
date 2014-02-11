#ifdef CONFIG_PARAVIRT_XEN
#include <xen/public/evtchn.h>
#else
/******************************************************************************
 * evtchn.h
 *
 * Interface to /dev/xen/evtchn.
 *
 * Copyright (c) 2003-2005, K A Fraser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __LINUX_PUBLIC_EVTCHN_H__
#define __LINUX_PUBLIC_EVTCHN_H__

/*
 * Bind a fresh port to VIRQ @virq.
 * Return allocated port.
 */
#define IOCTL_EVTCHN_BIND_VIRQ				\
	_IOC(_IOC_NONE, 'E', 0, sizeof(struct ioctl_evtchn_bind_virq))
struct ioctl_evtchn_bind_virq {
	unsigned int virq;
};

#ifdef CONFIG_XEN
struct irq_cfg {
	u32 info;
	union {
		int bindcount; /* for dynamic IRQs */
#ifdef CONFIG_X86_IO_APIC
		u8 vector; /* for physical IRQs */
#endif
	};
};
struct irq_cfg *alloc_irq_and_cfg_at(unsigned int at, int node);
static inline int evtchn_make_refcounted(unsigned int evtchn) { return 0; }
#endif

/*
 * Bind a fresh port to remote <@remote_domain, @remote_port>.
 * Return allocated port.
 */
#define IOCTL_EVTCHN_BIND_INTERDOMAIN			\
	_IOC(_IOC_NONE, 'E', 1, sizeof(struct ioctl_evtchn_bind_interdomain))
struct ioctl_evtchn_bind_interdomain {
	unsigned int remote_domain, remote_port;
};

/*
 * Allocate a fresh port for binding to @remote_domain.
 * Return allocated port.
 */
#define IOCTL_EVTCHN_BIND_UNBOUND_PORT			\
	_IOC(_IOC_NONE, 'E', 2, sizeof(struct ioctl_evtchn_bind_unbound_port))
struct ioctl_evtchn_bind_unbound_port {
	unsigned int remote_domain;
};

/*
 * Unbind previously allocated @port.
 */
#define IOCTL_EVTCHN_UNBIND				\
	_IOC(_IOC_NONE, 'E', 3, sizeof(struct ioctl_evtchn_unbind))
struct ioctl_evtchn_unbind {
	unsigned int port;
};

static inline void
multi_notify_remote_via_evtchn(multicall_entry_t *mcl, int port)
{
	struct evtchn_send *send = (void *)(mcl->args + 2);

	BUILD_BUG_ON(sizeof(*send) > sizeof(mcl->args) - 2 * sizeof(*mcl->args));
	send->port = port;
	mcl->op = __HYPERVISOR_event_channel_op;
	mcl->args[0] = EVTCHNOP_send;
	mcl->args[1] = (unsigned long)send;
}

static inline int close_evtchn(int port)
{
	struct evtchn_close close = { .port = port };
	return HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
}

/* Test an irq's pending state. */
int xen_test_irq_pending(int irq);

/*
 * Unbind previously allocated @port.
 */
#define IOCTL_EVTCHN_NOTIFY				\
	_IOC(_IOC_NONE, 'E', 4, sizeof(struct ioctl_evtchn_notify))
struct ioctl_evtchn_notify {
	unsigned int port;
};

/* Clear and reinitialise the event buffer. Clear error condition. */
#define IOCTL_EVTCHN_RESET				\
	_IOC(_IOC_NONE, 'E', 5, 0)

#endif /* __LINUX_PUBLIC_EVTCHN_H__ */
