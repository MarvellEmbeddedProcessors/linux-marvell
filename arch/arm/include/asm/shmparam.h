#ifndef _ASMARM_SHMPARAM_H
#define _ASMARM_SHMPARAM_H

/*
 * This should be the size of the virtually indexed cache/ways,
 * or page size, whichever is greater since the cache aliases
 * every size/ways bytes.
 */
#ifdef CONFIG_MV_LARGE_PAGE_SUPPORT
#define	SHMLBA	(16 << 10)		 /* attach addr a multiple of (4 * 4096) */
#else
#define	SHMLBA	(4 * PAGE_SIZE)		 /* attach addr a multiple of this */
#endif

/*
 * Enforce SHMLBA in shmat
 */
#define __ARCH_FORCE_SHMLBA

#endif /* _ASMARM_SHMPARAM_H */
