/****************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/string.h>

#include <net/gnss/mv_nss_ops.h>

#define MV_NSS_OPS_PREFIX_MAX		(32)
#define MV_NSS_OPS_PREFIX_NUM_OK(v)	((v) >= 0 && (v) < MV_NSS_OPS_PREFIX_MAX)
#define MV_NSS_OPS_PREFIX_SIZE_OK(v)	((v) > 0 && (v) < IFNAMSIZ)

#define MV_NSS_OPS_LOCK_SMP(nss_ops_lock, flags)					\
do {								\
	if (in_interrupt())					\
		spin_lock((nss_ops_lock));				\
	else							\
		spin_lock_irqsave((nss_ops_lock), (flags));		\
} while (0)

#define MV_NSS_OPS_UNLOCK_SMP(nss_ops_lock, flags)				\
do {								\
	if (in_interrupt())					\
		spin_unlock((nss_ops_lock));				\
	else							\
		spin_unlock_irqrestore((nss_ops_lock), (flags));	\
} while (0)

struct mv_nss_ops_prefix {
	char			prefix[IFNAMSIZ];
	struct list_head	list_elem;
};

static struct mv_nss_ops	*nss_ops;
static struct list_head		nss_ops_prefix_list = LIST_HEAD_INIT(nss_ops_prefix_list);
static int			nss_ops_list_size;
static bool			nss_ops_if_filter;
static spinlock_t		nss_ops_lock;

static struct mv_nss_ops_prefix *mv_nss_prefix_find(const char *name)
{
	struct list_head *curr;
	struct list_head *q;
	struct mv_nss_ops_prefix *tmp;

	list_for_each_safe(curr, q, &nss_ops_prefix_list) {
		tmp = list_entry(curr, struct mv_nss_ops_prefix, list_elem);
		pr_info("prefix:%s(%d) name:%s(%d)\n", tmp->prefix, strlen(tmp->prefix), name, strlen(name));
		if (!memcmp(tmp->prefix, name, strlen(tmp->prefix)))
			return tmp;
	}

	pr_info("not found name:%s(%d)\n", name, strlen(name));
	return NULL;
}

static struct mv_nss_ops_prefix *mv_nss_prefix_find_exact(const char *name)
{
	struct list_head *curr;
	struct list_head *q;
	struct mv_nss_ops_prefix *tmp;

	list_for_each_safe(curr, q, &nss_ops_prefix_list) {
		tmp = list_entry(curr, struct mv_nss_ops_prefix, list_elem);
		if (!strncmp(tmp->prefix, name, IFNAMSIZ))
			return tmp;
	}

	return NULL;
}

void mv_nss_ops_prefix_list_clear(void)
{
	struct list_head		*curr, *q;
	struct mv_nss_ops_prefix	*tmp;
	unsigned long iflags		= 0;

	MV_NSS_OPS_LOCK_SMP(&nss_ops_lock, iflags);
	list_for_each_safe(curr, q, &nss_ops_prefix_list) {
		tmp = list_entry(curr, struct mv_nss_ops_prefix, list_elem);
		list_del(&tmp->list_elem);
		kfree(tmp);
		nss_ops_list_size--;
	}
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);
}
EXPORT_SYMBOL(mv_nss_ops_prefix_list_clear);

void mv_nss_ops_prefix_list_show(void)
{
	struct list_head		*curr;
	struct mv_nss_ops_prefix	*tmp;

	pr_info("NSS PREFIX LIST SIZE=%d\n", nss_ops_list_size);
	list_for_each(curr, &nss_ops_prefix_list) {
		tmp = list_entry(curr, struct mv_nss_ops_prefix, list_elem);
		pr_info("%s\n", tmp->prefix);
	}
}
EXPORT_SYMBOL(mv_nss_ops_prefix_list_show);

void mv_nss_ops_show(void)
{
	pr_cont("NSS prefix filter:      %s\n", nss_ops_if_filter ? "ON" : "OFF");
	if (!nss_ops) {
		pr_info("\nEmpty OPS\n");
		return;
	}

	pr_info("\nNSS OPS:              %p\n", nss_ops);
	pr_cont("ops->alloc_skb:          %p\n", nss_ops->alloc_skb);
	pr_cont("ops->free_skb:           %p\n", nss_ops->free_skb);
	pr_cont("ops->receive_skb:        %p\n", nss_ops->receive_skb);
	pr_cont("ops->get_metadata_skb:   %p\n", nss_ops->get_metadata_skb);
	pr_cont("ops->init_metadata_skb:  %p\n", nss_ops->init_metadata_skb);
	pr_cont("ops->remove_metadata_skb:%p\n", nss_ops->remove_metadata_skb);
	pr_cont("ops->xmit_pause:         %p\n", nss_ops->xmit_pause);
	pr_cont("ops->xmit_resume:        %p\n", nss_ops->xmit_resume);
	pr_cont("ops->register_iface:     %p\n", nss_ops->register_iface);
	pr_cont("ops->unregister_iface:   %p\n", nss_ops->unregister_iface);
}
EXPORT_SYMBOL(mv_nss_ops_show);

int mv_nss_ops_prefix_add(const char *name)
{
	struct mv_nss_ops_prefix	*tmp;
	size_t				len;
	unsigned long iflags		= 0;

	if (!name) {
		pr_err("nss ops null prefix name\n");
		return -ENXIO;
	}

	len = strlen(name);
	if (!MV_NSS_OPS_PREFIX_SIZE_OK(len)) {
		pr_err("nss ops illegal prefix size: %d\n", len);
		return -ENXIO;
	}

	if (!MV_NSS_OPS_PREFIX_NUM_OK(nss_ops_list_size)) {
		pr_err("nss ops to many preficies: %d\n", nss_ops_list_size);
		return -EINVAL;
	}

	tmp = kzalloc(sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	strncpy(tmp->prefix, name, IFNAMSIZ);

	MV_NSS_OPS_LOCK_SMP(&nss_ops_lock, iflags);
	/*find already added*/
	if (unlikely(mv_nss_prefix_find_exact(name))) {
		pr_err("%s already added\n", name);
		goto err;
	}
	list_add_tail(&tmp->list_elem, &nss_ops_prefix_list);
	nss_ops_list_size++;
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);

	return 0;

err:
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);
	kfree(tmp);
	return -EINVAL;
}
EXPORT_SYMBOL(mv_nss_ops_prefix_add);

int mv_nss_ops_prefix_del(const char *name)
{
	struct mv_nss_ops_prefix	*tmp;
	unsigned long iflags		= 0;

	if (!name) {
		pr_err("nss ops null prefix name\n");
		return -ENXIO;
	}

	MV_NSS_OPS_LOCK_SMP(&nss_ops_lock, iflags);
	tmp = mv_nss_prefix_find_exact(name);
	if (unlikely(!tmp)) {
		pr_err("%s not found\n", name);
		goto err;
	}

	list_del(&tmp->list_elem);
	nss_ops_list_size--;
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);

	kfree(tmp);

	return 0;

err:
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);
	return -EINVAL;
}
EXPORT_SYMBOL(mv_nss_ops_prefix_del);

struct mv_nss_ops *mv_nss_ops_get(const struct net_device *iface)
{
	if (!nss_ops_if_filter) {
		pr_info("nss_ops_get:%p interface:%s\n", nss_ops, iface ? iface->name : "NULL");
		return nss_ops;
	}

	if (!iface)
		goto not_found;

	if (!mv_nss_prefix_find(iface->name))
		goto not_found;

	pr_info("nss_ops_get:%p interface:%s\n", nss_ops, iface->name);
	return nss_ops;

not_found:
	pr_devel("debug: nss_ops_get: interface:%s not supported\n", iface ? iface->name : "NULL");
	return NULL;
}
EXPORT_SYMBOL(mv_nss_ops_get);

void mv_nss_ops_set(struct mv_nss_ops *ops)
{
	nss_ops = ops;
	pr_info("nss ops_set ops:%p\n", nss_ops);
}
EXPORT_SYMBOL(mv_nss_ops_set);

void mv_nss_ops_filter_on(bool filter)
{
	unsigned long iflags		= 0;

	MV_NSS_OPS_LOCK_SMP(&nss_ops_lock, iflags);
	nss_ops_if_filter = filter;
	MV_NSS_OPS_UNLOCK_SMP(&nss_ops_lock, iflags);
}
EXPORT_SYMBOL(mv_nss_ops_filter_on);

