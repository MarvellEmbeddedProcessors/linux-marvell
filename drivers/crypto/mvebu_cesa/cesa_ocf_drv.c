/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/random.h>
#include <asm/scatterlist.h>
#include <linux/spinlock.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "cesa_if.h" /* moved here before cryptodev.h due to include dependencies */
#include <cryptodev.h>
#include <uio.h>

#include "mvDebug.h"

#include "mvMD5.h"
#include "mvSHA1.h"

#include "mvCesaRegs.h"
#include "AES/mvAes.h"
#include "mvLru.h"

#ifndef CONFIG_OF
#error cesa_ocf driver supports only DT configuration
#endif


#undef RT_DEBUG
#ifdef RT_DEBUG
static int debug = 1;
module_param(debug, int, 1);
MODULE_PARM_DESC(debug, "Enable debug");
#undef dprintk
#define dprintk(a...)	if (debug) { printk(a); } else
#else
static int debug = 0;
#undef dprintk
#define dprintk(a...)
#endif

#define	DRIVER_NAME	"armada-cesa-ocf"

/* interrupt handling */
#undef CESA_OCF_TASKLET

extern int cesaReqResources[MV_CESA_CHANNELS];
/* support for spliting action into 2 actions */
#define CESA_OCF_SPLIT

/* general defines */
#define CESA_OCF_MAX_SES 	128
#define CESA_Q_SIZE		256
#define CESA_RESULT_Q_SIZE	(CESA_Q_SIZE * MV_CESA_CHANNELS * 2)
#define CESA_OCF_POOL_SIZE	(CESA_Q_SIZE * MV_CESA_CHANNELS * 2)

/* data structures */
struct cesa_ocf_data {
        int                                      cipher_alg;
        int                                      auth_alg;
	int					 encrypt_tn_auth;
#define  auth_tn_decrypt  encrypt_tn_auth
	int					 ivlen;
	int 					 digestlen;
	short					 sid_encrypt;
	short					 sid_decrypt;
	/* fragment workaround sessions */
	short					 frag_wa_encrypt;
	short					 frag_wa_decrypt;
	short					 frag_wa_auth;
};

#define DIGEST_BUF_SIZE	32
struct cesa_ocf_process {
	MV_CESA_COMMAND 			cesa_cmd;
	MV_CESA_MBUF 				cesa_mbuf;
	MV_BUF_INFO  				cesa_bufs[MV_CESA_MAX_MBUF_FRAGS];
	char					digest[DIGEST_BUF_SIZE];
	int					digest_len;
	struct cryptop 				*crp;
	int 					need_cb;
};

/* global variables */
static int32_t			cesa_ocf_id 		= -1;
static struct cesa_ocf_data 	**cesa_ocf_sessions = NULL;
static u_int32_t		cesa_ocf_sesnum = 0;
static DEFINE_SPINLOCK(cesa_lock);
static atomic_t result_count;
static struct cesa_ocf_process *result_Q[CESA_RESULT_Q_SIZE];
static unsigned int next_result;
static unsigned int result_done;
static unsigned char chan_id[MV_CESA_CHANNELS];

/* static APIs */
static int 		cesa_ocf_process	(device_t, struct cryptop *, int);
static int 		cesa_ocf_newsession	(device_t, u_int32_t *, struct cryptoini *);
static int 		cesa_ocf_freesession	(device_t, u_int64_t);
static inline void 	cesa_callback		(unsigned long);
static irqreturn_t	cesa_interrupt_handler	(int, void *);
#ifdef CESA_OCF_TASKLET
static struct tasklet_struct cesa_ocf_tasklet;
#endif

static struct timeval          tt_start;
static struct timeval          tt_end;
static struct cesa_ocf_process *cesa_ocf_pool = NULL;
static struct cesa_ocf_process *cesa_ocf_stack[CESA_OCF_POOL_SIZE];
static unsigned int cesa_ocf_stack_idx;

/*
 * dummy device structure
 */
static struct {
	softc_device_decl	sc_dev;
} mv_cesa_dev;

static device_method_t mv_cesa_methods = {
	/* crypto device methods */
	DEVMETHOD(cryptodev_newsession,	cesa_ocf_newsession),
	DEVMETHOD(cryptodev_freesession,cesa_ocf_freesession),
	DEVMETHOD(cryptodev_process,	cesa_ocf_process),
	DEVMETHOD(cryptodev_kprocess,	NULL),
};

unsigned int
get_usec(unsigned int start)
{
	if(start) {
		do_gettimeofday (&tt_start);
		return 0;
	}
	else {
		do_gettimeofday (&tt_end);
		tt_end.tv_sec -= tt_start.tv_sec;
		tt_end.tv_usec -= tt_start.tv_usec;
		if (tt_end.tv_usec < 0) {
			tt_end.tv_usec += 1000 * 1000;
			tt_end.tv_sec -= 1;
		}
	}
	printk("time taken is  %d\n", (unsigned int)(tt_end.tv_usec + tt_end.tv_sec * 1000000));
	return (tt_end.tv_usec + tt_end.tv_sec * 1000000);
}

static void
skb_copy_bits_back(struct sk_buff *skb, int offset, caddr_t cp, int len)
{
        int i;
        if (offset < skb_headlen(skb)) {
                memcpy(skb->data + offset, cp, min_t(int, skb_headlen(skb), len));
                len -= skb_headlen(skb);
                cp += skb_headlen(skb);
        }
        offset -= skb_headlen(skb);
        for (i = 0; len > 0 && i < skb_shinfo(skb)->nr_frags; i++) {
                if (offset < skb_shinfo(skb)->frags[i].size) {
                        memcpy(page_address(skb_shinfo(skb)->frags[i].page.p) +
                                        skb_shinfo(skb)->frags[i].page_offset,
                                        cp, min_t(int, skb_shinfo(skb)->frags[i].size, len));
                        len -= skb_shinfo(skb)->frags[i].size;
                        cp += skb_shinfo(skb)->frags[i].size;
                }
                offset -= skb_shinfo(skb)->frags[i].size;
        }
}


#ifdef RT_DEBUG
/*
 * check that the crp action match the current session
 */
static int
ocf_check_action(struct cryptop *crp, struct cesa_ocf_data *cesa_ocf_cur_ses) {
	int count = 0;
	int encrypt = 0, decrypt = 0, auth = 0;
	struct cryptodesc *crd;

        /* Go through crypto descriptors, processing as we go */
        for (crd = crp->crp_desc; crd; crd = crd->crd_next, count++) {
		if(count > 2) {
			printk("%s,%d: session mode is not supported.\n", __FILE__, __LINE__);
			return 1;
		}

		/* Encryption /Decryption */
		if(crd->crd_alg == cesa_ocf_cur_ses->cipher_alg) {
			/* check that the action is compatible with session */
			if(encrypt || decrypt) {
				printk("%s,%d: session mode is not supported.\n", __FILE__, __LINE__);
				return 1;
			}

			if(crd->crd_flags & CRD_F_ENCRYPT) { /* encrypt */
				if( (count == 2) && (cesa_ocf_cur_ses->encrypt_tn_auth) ) {
					printk("%s,%d: sequence isn't supported by this session.\n", __FILE__, __LINE__);
					return 1;
				}
				encrypt++;
			}
			else { 					/* decrypt */
				if( (count == 2) && !(cesa_ocf_cur_ses->auth_tn_decrypt) ) {
					printk("%s,%d: sequence isn't supported by this session.\n", __FILE__, __LINE__);
					return 1;
				}
				decrypt++;
			}

		}
		/* Authentication */
		else if(crd->crd_alg == cesa_ocf_cur_ses->auth_alg) {
			/* check that the action is compatible with session */
			if(auth) {
				printk("%s,%d: session mode is not supported.\n", __FILE__, __LINE__);
				return 1;
			}
			if( (count == 2) && (decrypt) && (cesa_ocf_cur_ses->auth_tn_decrypt)) {
				printk("%s,%d: sequence isn't supported by this session.\n", __FILE__, __LINE__);
				return 1;
			}
			if( (count == 2) && (encrypt) && !(cesa_ocf_cur_ses->encrypt_tn_auth)) {
				printk("%s,%d: sequence isn't supported by this session.\n", __FILE__, __LINE__);
				return 1;
			}
			auth++;
		}
		else {
			printk("%s,%d: Alg isn't supported by this session.\n", __FILE__, __LINE__);
			return 1;
		}
	}
	return 0;

}
#endif

static inline struct cesa_ocf_process* cesa_ocf_alloc(void)
{
	struct cesa_ocf_process *retval;
	unsigned long flags;

	spin_lock_irqsave(&cesa_lock, flags);
	if (cesa_ocf_stack_idx == 0) {
		spin_unlock_irqrestore(&cesa_lock, flags);
		return NULL;
	}

	retval = cesa_ocf_stack[--cesa_ocf_stack_idx];
	spin_unlock_irqrestore(&cesa_lock, flags);

	return retval;
}

static inline void cesa_ocf_free(struct cesa_ocf_process *ocf_process_p)
{
	unsigned long flags;

	spin_lock_irqsave(&cesa_lock, flags);
	cesa_ocf_stack[cesa_ocf_stack_idx++] = ocf_process_p;
	spin_unlock_irqrestore(&cesa_lock, flags);
}


/*
 * Process a request.
 */
static int
cesa_ocf_process(device_t dev, struct cryptop *crp, int hint)
{
	struct cesa_ocf_process *cesa_ocf_cmd = NULL;
	struct cesa_ocf_process *cesa_ocf_cmd_wa = NULL;
	MV_CESA_COMMAND	*cesa_cmd;
	struct cryptodesc *crd;
	struct cesa_ocf_data *cesa_ocf_cur_ses;
	int sid = 0, temp_len = 0, i;
	int encrypt = 0, decrypt = 0, auth = 0;
	int  status, free_resrc = 0;
	struct sk_buff *skb = NULL;
	struct uio *uiop = NULL;
	unsigned char *ivp;
	MV_BUF_INFO *p_buf_info;
	MV_CESA_MBUF *p_mbuf_info;
	unsigned long flags;
	unsigned char chan = 0;


        dprintk("%s()\n", __func__);

	for (chan = 0; chan < mv_cesa_channels; chan++)
		free_resrc += cesaReqResources[chan];

		/* In case request should be split, at least 2 slots
			should be available in CESA fifo */
		if (free_resrc < 2) {
			dprintk("%s,%d: ERESTART\n", __FILE__, __LINE__);
			return -ERESTART;
		}

#ifdef RT_DEBUG
        /* Sanity check */
        if (crp == NULL) {
                printk("%s,%d: EINVAL\n", __FILE__, __LINE__);
		return -EINVAL;
        }

        if (crp->crp_desc == NULL || crp->crp_buf == NULL ) {
                printk("%s,%d: EINVAL\n", __FILE__, __LINE__);
                crp->crp_etype = EINVAL;
		return -EINVAL;
        }

        sid = crp->crp_sid & 0xffffffff;
        if ((sid >= cesa_ocf_sesnum) || (cesa_ocf_sessions[sid] == NULL)) {
		crp->crp_etype = -ENOENT;
		printk(KERN_ERR "%s,%d: ENOENT session %d\n", __FILE__,
				__LINE__, sid);
		return -EINVAL;
        }
#endif

	sid = crp->crp_sid & 0xffffffff;
	crp->crp_etype = 0;
	cesa_ocf_cur_ses = cesa_ocf_sessions[sid];

#ifdef RT_DEBUG
	if(ocf_check_action(crp, cesa_ocf_cur_ses)){
		goto p_error;
	}
#endif

	/* Allocate new cesa process from local pool */
	cesa_ocf_cmd = cesa_ocf_alloc();
	if (cesa_ocf_cmd == NULL) {
		printk("%s,%d: ENOBUFS \n", __FILE__, __LINE__);
		goto p_error;
	}

	/* init cesa_process */
	cesa_ocf_cmd->crp = crp;
	/* always call callback */
	cesa_ocf_cmd->need_cb = 1;

	/* init cesa_cmd for usage of the HALs */
	cesa_cmd = &cesa_ocf_cmd->cesa_cmd;
	cesa_cmd->pReqPrv = (void *)cesa_ocf_cmd;
	cesa_cmd->sessionId = cesa_ocf_cur_ses->sid_encrypt; /* defualt use encrypt */

	/* prepare src buffer 	*/
	/* we send the entire buffer to the HAL, even if only part of it should be encrypt/auth.  */
	/* if not using seesions for both encrypt and auth, then it will be wiser to to copy only */
	/* from skip to crd_len. 								  */
	p_buf_info = cesa_ocf_cmd->cesa_bufs;
	p_mbuf_info = &cesa_ocf_cmd->cesa_mbuf;

	p_buf_info += 2; /* save 2 first buffers for IV and digest -
			    we won't append them to the end since, they
			    might be places in an unaligned addresses. */

	p_mbuf_info->pFrags = p_buf_info;
	temp_len = 0;

	/* handle SKB */
	if (crp->crp_flags & CRYPTO_F_SKBUF) {

		dprintk("%s,%d: handle SKB.\n", __FILE__, __LINE__);
		skb = (struct sk_buff *) crp->crp_buf;

                if (skb_shinfo(skb)->nr_frags >= (MV_CESA_MAX_MBUF_FRAGS - 1)) {
                        printk("%s,%d: %d nr_frags > MV_CESA_MAX_MBUF_FRAGS", __FILE__, __LINE__, skb_shinfo(skb)->nr_frags);
                        goto p_error;
                }

		p_mbuf_info->mbufSize = skb->len;
		temp_len = skb->len;
		/* first skb fragment */
		p_buf_info->bufSize = skb_headlen(skb);
		p_buf_info->bufVirtPtr = skb->data;
		p_buf_info++;

		/* now handle all other skb fragments */
		for ( i = 0; i < skb_shinfo(skb)->nr_frags; i++ ) {
			skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
			p_buf_info->bufSize = frag->size;
			p_buf_info->bufVirtPtr = page_address(frag->page.p) + frag->page_offset;
			p_buf_info++;
		}
		p_mbuf_info->numFrags = skb_shinfo(skb)->nr_frags + 1;
	}
	/* handle UIO */
	else if(crp->crp_flags & CRYPTO_F_IOV) {

		dprintk("%s,%d: handle UIO.\n", __FILE__, __LINE__);
		uiop = (struct uio *) crp->crp_buf;

                if (uiop->uio_iovcnt > (MV_CESA_MAX_MBUF_FRAGS - 1)) {
                        printk("%s,%d: %d uio_iovcnt > MV_CESA_MAX_MBUF_FRAGS \n", __FILE__, __LINE__, uiop->uio_iovcnt);
                        goto p_error;
                }

		p_mbuf_info->mbufSize = crp->crp_ilen;
		p_mbuf_info->numFrags = uiop->uio_iovcnt;
		for(i = 0; i < uiop->uio_iovcnt; i++) {
			p_buf_info->bufVirtPtr = uiop->uio_iov[i].iov_base;
			p_buf_info->bufSize = uiop->uio_iov[i].iov_len;
			temp_len += p_buf_info->bufSize;
			dprintk("%s,%d: buf %x-> addr %x, size %x \n"
				, __FILE__, __LINE__, i, (unsigned int)p_buf_info->bufVirtPtr, p_buf_info->bufSize);
			p_buf_info++;
		}

	}
	/* handle CONTIG */
	else {
		dprintk("%s,%d: handle CONTIG.\n", __FILE__, __LINE__);
		p_mbuf_info->numFrags = 1;
		p_mbuf_info->mbufSize = crp->crp_ilen;
		p_buf_info->bufVirtPtr = crp->crp_buf;
		p_buf_info->bufSize = crp->crp_ilen;
		temp_len = crp->crp_ilen;
		p_buf_info++;
	}

	/* Support up to 64K why? cause! */
	if(crp->crp_ilen > 64*1024) {
		printk("%s,%d: buf too big %x \n", __FILE__, __LINE__, crp->crp_ilen);
		goto p_error;
	}

	if( temp_len != crp->crp_ilen ) {
		printk("%s,%d: warning size don't match.(%x %x) \n", __FILE__, __LINE__, temp_len, crp->crp_ilen);
	}

	cesa_cmd->pSrc = p_mbuf_info;
	cesa_cmd->pDst = p_mbuf_info;

	/* restore p_buf_info to point to first available buf */
	p_buf_info = cesa_ocf_cmd->cesa_bufs;
	p_buf_info += 1;


        /* Go through crypto descriptors, processing as we go */
        for (crd = crp->crp_desc; crd; crd = crd->crd_next) {

		/* Encryption /Decryption */
		if(crd->crd_alg == cesa_ocf_cur_ses->cipher_alg) {

			dprintk("%s,%d: cipher", __FILE__, __LINE__);

			cesa_cmd->cryptoOffset = crd->crd_skip;
			cesa_cmd->cryptoLength = crd->crd_len;

			if(crd->crd_flags & CRD_F_ENCRYPT) { /* encrypt */
				dprintk(" encrypt \n");
				encrypt++;

				/* handle IV */
				if (crd->crd_flags & CRD_F_IV_EXPLICIT) {  /* IV from USER */
					dprintk("%s,%d: IV from USER (offset %x) \n", __FILE__, __LINE__, crd->crd_inject);
					cesa_cmd->ivFromUser = 1;
					ivp = crd->crd_iv;

					/*
					 * do we have to copy the IV back to the buffer ?
					 */
					if ((crd->crd_flags & CRD_F_IV_PRESENT) == 0) {
						dprintk("%s,%d: copy the IV back to the buffer\n", __FILE__, __LINE__);
						cesa_cmd->ivOffset = crd->crd_inject;
						if (crp->crp_flags & CRYPTO_F_SKBUF)
							skb_copy_bits_back(skb, crd->crd_inject, ivp, cesa_ocf_cur_ses->ivlen);
						else if (crp->crp_flags & CRYPTO_F_IOV)
							cuio_copyback(uiop,crd->crd_inject, cesa_ocf_cur_ses->ivlen,(caddr_t)ivp);
						else
							memcpy(crp->crp_buf + crd->crd_inject, ivp, cesa_ocf_cur_ses->ivlen);
					}
					else {
						dprintk("%s,%d: don't copy the IV back to the buffer \n", __FILE__, __LINE__);
						p_mbuf_info->numFrags++;
						p_mbuf_info->mbufSize += cesa_ocf_cur_ses->ivlen;
						p_mbuf_info->pFrags = p_buf_info;

						p_buf_info->bufVirtPtr = ivp;
						p_buf_info->bufSize = cesa_ocf_cur_ses->ivlen;
						p_buf_info--;

						/* offsets */
						cesa_cmd->ivOffset = 0;
						cesa_cmd->cryptoOffset += cesa_ocf_cur_ses->ivlen;
						if(auth) {
							cesa_cmd->macOffset += cesa_ocf_cur_ses->ivlen;
							cesa_cmd->digestOffset += cesa_ocf_cur_ses->ivlen;
						}
					}
                                }
				else {					/* random IV */
					dprintk("%s,%d: random IV \n", __FILE__, __LINE__);
					cesa_cmd->ivFromUser = 0;

					/*
					 * do we have to copy the IV back to the buffer ?
					 */
					/* in this mode the HAL will always copy the IV */
					/* given by the session to the ivOffset  	*/
					if ((crd->crd_flags & CRD_F_IV_PRESENT) == 0) {
						cesa_cmd->ivOffset = crd->crd_inject;
					}
					else {
						/* if IV isn't copy, then how will the user know which IV did we use??? */
						printk("%s,%d: EINVAL\n", __FILE__, __LINE__);
						goto p_error;
					}
				}
			}
			else { 					/* decrypt */
				dprintk(" decrypt \n");
				decrypt++;
				cesa_cmd->sessionId = cesa_ocf_cur_ses->sid_decrypt;

				/* handle IV */
				if (crd->crd_flags & CRD_F_IV_EXPLICIT) {
					dprintk("%s,%d: IV from USER \n", __FILE__, __LINE__);
					/* append the IV buf to the mbuf */
					cesa_cmd->ivFromUser = 1;
					p_mbuf_info->numFrags++;
					p_mbuf_info->mbufSize += cesa_ocf_cur_ses->ivlen;
					p_mbuf_info->pFrags = p_buf_info;

					p_buf_info->bufVirtPtr = crd->crd_iv;
					p_buf_info->bufSize = cesa_ocf_cur_ses->ivlen;
					p_buf_info--;

					/* offsets */
					cesa_cmd->ivOffset = 0;
					cesa_cmd->cryptoOffset += cesa_ocf_cur_ses->ivlen;
					if(auth) {
						cesa_cmd->macOffset += cesa_ocf_cur_ses->ivlen;
						cesa_cmd->digestOffset += cesa_ocf_cur_ses->ivlen;
					}
                                }
				else {
					dprintk("%s,%d: IV inside the buffer \n", __FILE__, __LINE__);
					cesa_cmd->ivFromUser = 0;
					cesa_cmd->ivOffset = crd->crd_inject;
				}
			}

		}
		/* Authentication */
		else if(crd->crd_alg == cesa_ocf_cur_ses->auth_alg) {
			dprintk("%s,%d:  Authentication \n", __FILE__, __LINE__);
			auth++;
			cesa_cmd->macOffset = crd->crd_skip;
			cesa_cmd->macLength = crd->crd_len;

			/* digest + mac */
			cesa_cmd->digestOffset = crd->crd_inject;
		}
		else {
			printk("%s,%d: Alg isn't supported by this session.\n", __FILE__, __LINE__);
			goto p_error;
		}
	}

	dprintk("\n");
	dprintk("%s,%d: Sending Action: \n", __FILE__, __LINE__);
	dprintk("%s,%d: IV from user: %d. IV offset %x \n",  __FILE__, __LINE__, cesa_cmd->ivFromUser, cesa_cmd->ivOffset);
	dprintk("%s,%d: crypt offset %x len %x \n", __FILE__, __LINE__, cesa_cmd->cryptoOffset, cesa_cmd->cryptoLength);
	dprintk("%s,%d: Auth offset %x len %x \n", __FILE__, __LINE__, cesa_cmd->macOffset, cesa_cmd->macLength);
	dprintk("%s,%d: set digest in offset %x . \n", __FILE__, __LINE__, cesa_cmd->digestOffset);
	if(debug) {
		mvCesaIfDebugMbuf("SRC BUFFER", cesa_cmd->pSrc, 0, cesa_cmd->pSrc->mbufSize);
	}

	cesa_cmd->split = MV_CESA_SPLIT_NONE;

	/* send action to HAL */
	spin_lock_irqsave(&cesa_lock, flags);
	status = mvCesaIfAction(cesa_cmd);
	spin_unlock_irqrestore(&cesa_lock, flags);

	/* action not allowed */
	if(status == MV_NOT_ALLOWED) {
#ifdef CESA_OCF_SPLIT
		/* if both encrypt and auth try to split */
		if(auth && (encrypt || decrypt)) {
			MV_CESA_COMMAND	*cesa_cmd_wa;

			/* Allocate new cesa process from local pool and initialize it */
			cesa_ocf_cmd_wa = cesa_ocf_alloc();

			if (cesa_ocf_cmd_wa == NULL) {
				printk("%s,%d: ENOBUFS \n", __FILE__, __LINE__);
				goto p_error;
			}
			memcpy(cesa_ocf_cmd_wa, cesa_ocf_cmd, sizeof(struct cesa_ocf_process));
			cesa_cmd_wa = &cesa_ocf_cmd_wa->cesa_cmd;
			cesa_cmd_wa->pReqPrv = (void *)cesa_ocf_cmd_wa;
			cesa_ocf_cmd_wa->need_cb = 0;
			cesa_cmd_wa->split = MV_CESA_SPLIT_FIRST;
			cesa_cmd->split = MV_CESA_SPLIT_SECOND;

			/* break requests to two operation, first operation completion won't call callback */
			if((decrypt) && (cesa_ocf_cur_ses->auth_tn_decrypt)) {
				cesa_cmd_wa->sessionId = cesa_ocf_cur_ses->frag_wa_auth;
				cesa_cmd->sessionId = cesa_ocf_cur_ses->frag_wa_decrypt;
			}
			else if((decrypt) && !(cesa_ocf_cur_ses->auth_tn_decrypt)) {
				cesa_cmd_wa->sessionId = cesa_ocf_cur_ses->frag_wa_decrypt;
				cesa_cmd->sessionId = cesa_ocf_cur_ses->frag_wa_auth;
			}
			else if((encrypt) && (cesa_ocf_cur_ses->encrypt_tn_auth)) {
				cesa_cmd_wa->sessionId = cesa_ocf_cur_ses->frag_wa_encrypt;
				cesa_cmd->sessionId = cesa_ocf_cur_ses->frag_wa_auth;
			}
			else if((encrypt) && !(cesa_ocf_cur_ses->encrypt_tn_auth)){
				cesa_cmd_wa->sessionId = cesa_ocf_cur_ses->frag_wa_auth;
				cesa_cmd->sessionId = cesa_ocf_cur_ses->frag_wa_encrypt;
			}
			else {
				printk("%s,%d: Unsupporterd fragment wa mode \n", __FILE__, __LINE__);
				goto p_error;
			}

			/* send the 2 actions to the HAL */
			spin_lock_irqsave(&cesa_lock, flags);
			status = mvCesaIfAction(cesa_cmd_wa);
			spin_unlock_irqrestore(&cesa_lock, flags);

			if((status != MV_NO_MORE) && (status != MV_OK)) {
				printk("%s,%d: cesa action failed, status = 0x%x\n", __FILE__, __LINE__, status);
				goto p_error;
			}
			spin_lock_irqsave(&cesa_lock, flags);
			status = mvCesaIfAction(cesa_cmd);
			spin_unlock_irqrestore(&cesa_lock, flags);

		}
		/* action not allowed and can't split */
		else
#endif
		{
			goto p_error;
		}
	}

	/* Hal Q is full, send again. This should never happen */
	if(status == MV_NO_RESOURCE) {
		dprintk("%s,%d: cesa no more resources \n", __FILE__, __LINE__);
		if(cesa_ocf_cmd)
			cesa_ocf_free(cesa_ocf_cmd);
		if(cesa_ocf_cmd_wa)
			cesa_ocf_free(cesa_ocf_cmd_wa);

		return -ERESTART;
	} else if ((status != MV_NO_MORE) && (status != MV_OK)) {
                printk("%s,%d: cesa action failed, status = 0x%x\n", __FILE__, __LINE__, status);
		goto p_error;
        }

	dprintk("%s, status %d\n", __func__, status);

	return 0;

p_error:
	crp->crp_etype = -EINVAL;
	if(cesa_ocf_cmd)
		cesa_ocf_free(cesa_ocf_cmd);
	if(cesa_ocf_cmd_wa)
		cesa_ocf_free(cesa_ocf_cmd_wa);

	return -EINVAL;
}

/*
 * cesa callback.
 */
static inline void
cesa_callback(unsigned long dummy)
{
	struct cesa_ocf_process *cesa_ocf_cmd = NULL;
	struct cryptop 		*crp = NULL;
	int need_cb;

	dprintk("%s()\n", __func__);

	spin_lock(&cesa_lock);

	while (atomic_read(&result_count) != 0) {
		cesa_ocf_cmd = result_Q[result_done];
		need_cb = cesa_ocf_cmd->need_cb;
		crp = cesa_ocf_cmd->crp;

		if (debug && need_cb)
			mvCesaIfDebugMbuf("DST BUFFER", cesa_ocf_cmd->cesa_cmd.pDst, 0,
							cesa_ocf_cmd->cesa_cmd.pDst->mbufSize);

		result_done = ((result_done + 1) % CESA_RESULT_Q_SIZE);
		atomic_dec(&result_count);
		cesa_ocf_stack[cesa_ocf_stack_idx++] = cesa_ocf_cmd;

		if (need_cb) {
			spin_unlock(&cesa_lock);
			crypto_done(crp);
			spin_lock(&cesa_lock);
		}
	}

	spin_unlock(&cesa_lock);

	return;
}

/*
 * cesa Interrupt Service Routine.
 */
static irqreturn_t
cesa_interrupt_handler(int irq, void *arg)
{
	MV_CESA_RESULT  	result;
	MV_STATUS               status;
	unsigned int cause, mask;
	unsigned char chan = *((u8 *)arg);

	dprintk("%s()\n", __func__);

	if (mv_cesa_feature == INT_COALESCING)
		mask = MV_CESA_CAUSE_EOP_COAL_MASK;
	else
		mask = MV_CESA_CAUSE_ACC_DMA_MASK;

	/* Read cause register */
	cause = MV_REG_READ(MV_CESA_ISR_CAUSE_REG(chan));

	if (likely(cause & mask)) {

		spin_lock(&cesa_lock);

		/* Clear pending irq */
		MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);

		/* Get completed results */
		while (atomic_read(&result_count) < CESA_RESULT_Q_SIZE) {

			status = mvCesaIfReadyGet(chan, &result);
			if (status != MV_OK)
				break;

			result_Q[next_result] = (struct cesa_ocf_process *)result.pReqPrv;
			next_result = ((next_result + 1) % CESA_RESULT_Q_SIZE);
			atomic_inc(&result_count);
		}

		spin_unlock(&cesa_lock);

		if (atomic_read(&result_count) >= CESA_RESULT_Q_SIZE) {
			/* In case reaching this point -result_Q should be tuned   */
				printk("%s: Error: Q request is full(chan=%d)\n", __func__, chan);
				return IRQ_HANDLED;
		}
	}

	if(likely(atomic_read(&result_count) > 0))
#ifdef CESA_OCF_TASKLET
		tasklet_hi_schedule(&cesa_ocf_tasklet);
#else
		cesa_callback(0);
#endif

	return IRQ_HANDLED;
}

/*
 * Open a session.
 */
static int
/*cesa_ocf_newsession(void *arg, u_int32_t *sid, struct cryptoini *cri)*/
cesa_ocf_newsession(device_t dev, u_int32_t *sid, struct cryptoini *cri)
{
	u32 status = 0, i = 0;
	unsigned long flags = 0;
	u32 count = 0, auth = 0, encrypt =0;
	struct cesa_ocf_data *cesa_ocf_cur_ses;
	MV_CESA_OPEN_SESSION cesa_session;
	MV_CESA_OPEN_SESSION *cesa_ses = &cesa_session;


        dprintk("%s()\n", __func__);
        if (sid == NULL || cri == NULL) {
                printk("%s,%d: EINVAL\n", __FILE__, __LINE__);
                return EINVAL;
        }

	if (cesa_ocf_sessions) {
		for (i = 1; i < cesa_ocf_sesnum; i++)
			if (cesa_ocf_sessions[i] == NULL)
				break;
	} else
		i = 1;

	if (cesa_ocf_sessions == NULL || i == cesa_ocf_sesnum) {
		struct cesa_ocf_data **cesa_ocf_new_sessions;

		if (cesa_ocf_sessions == NULL) {
			i = 1; /* We leave cesa_ocf_sessions[0] empty */
			cesa_ocf_sesnum = CESA_OCF_MAX_SES;
		}
		else
			cesa_ocf_sesnum *= 2;

		cesa_ocf_new_sessions = kmalloc(cesa_ocf_sesnum * sizeof(struct cesa_ocf_data *), SLAB_ATOMIC);
		if (cesa_ocf_new_sessions == NULL) {
			/* Reset session number */
			if (cesa_ocf_sesnum == CESA_OCF_MAX_SES)
				cesa_ocf_sesnum = 0;
			else
				cesa_ocf_sesnum /= 2;
			printk("%s,%d: ENOBUFS\n", __FILE__, __LINE__);
			return ENOBUFS;
		}
		memset(cesa_ocf_new_sessions, 0, cesa_ocf_sesnum * sizeof(struct cesa_ocf_data *));

		/* Copy existing sessions */
		if (cesa_ocf_sessions) {
			memcpy(cesa_ocf_new_sessions, cesa_ocf_sessions,
			    (cesa_ocf_sesnum / 2) * sizeof(struct cesa_ocf_data *));
			kfree(cesa_ocf_sessions);
		}

		cesa_ocf_sessions = cesa_ocf_new_sessions;
	}

	cesa_ocf_sessions[i] = (struct cesa_ocf_data *) kmalloc(sizeof(struct cesa_ocf_data),
			SLAB_ATOMIC);
	if (cesa_ocf_sessions[i] == NULL) {
		cesa_ocf_freesession(NULL, i);
		dprintk("%s,%d: EINVAL\n", __FILE__, __LINE__);
		return ENOBUFS;
	}

	dprintk("%s,%d: new session %d \n", __FILE__, __LINE__, i);

        *sid = i;
        cesa_ocf_cur_ses = cesa_ocf_sessions[i];
        memset(cesa_ocf_cur_ses, 0, sizeof(struct cesa_ocf_data));
	cesa_ocf_cur_ses->sid_encrypt = -1;
	cesa_ocf_cur_ses->sid_decrypt = -1;
	cesa_ocf_cur_ses->frag_wa_encrypt = -1;
	cesa_ocf_cur_ses->frag_wa_decrypt = -1;
	cesa_ocf_cur_ses->frag_wa_auth = -1;

	/* init the session */
	memset(cesa_ses, 0, sizeof(MV_CESA_OPEN_SESSION));
	count = 1;
        while (cri) {
		if(count > 2) {
			printk("%s,%d: don't support more then 2 operations\n", __FILE__, __LINE__);
			goto error;
		}
                switch (cri->cri_alg) {
		case CRYPTO_AES_CBC:
			dprintk("%s,%d: (%d) AES CBC \n", __FILE__, __LINE__, count);
			cesa_ocf_cur_ses->cipher_alg = cri->cri_alg;
			cesa_ocf_cur_ses->ivlen = MV_CESA_AES_BLOCK_SIZE;
			cesa_ses->cryptoAlgorithm = MV_CESA_CRYPTO_AES;
			cesa_ses->cryptoMode = MV_CESA_CRYPTO_CBC;
			if(cri->cri_klen/8 > MV_CESA_MAX_CRYPTO_KEY_LENGTH) {
				printk("%s,%d: CRYPTO key too long.\n", __FILE__, __LINE__);
				goto error;
			}
			memcpy(cesa_ses->cryptoKey, cri->cri_key, cri->cri_klen/8);
			dprintk("%s,%d: key length %d \n", __FILE__, __LINE__, cri->cri_klen/8);
			cesa_ses->cryptoKeyLength = cri->cri_klen/8;
			encrypt += count;
			break;
                case CRYPTO_3DES_CBC:
			dprintk("%s,%d: (%d) 3DES CBC \n", __FILE__, __LINE__, count);
			cesa_ocf_cur_ses->cipher_alg = cri->cri_alg;
			cesa_ocf_cur_ses->ivlen = MV_CESA_3DES_BLOCK_SIZE;
			cesa_ses->cryptoAlgorithm = MV_CESA_CRYPTO_3DES;
			cesa_ses->cryptoMode = MV_CESA_CRYPTO_CBC;
			if(cri->cri_klen/8 > MV_CESA_MAX_CRYPTO_KEY_LENGTH) {
				printk("%s,%d: CRYPTO key too long.\n", __FILE__, __LINE__);
				goto error;
			}
			memcpy(cesa_ses->cryptoKey, cri->cri_key, cri->cri_klen/8);
			cesa_ses->cryptoKeyLength = cri->cri_klen/8;
			encrypt += count;
			break;
                case CRYPTO_DES_CBC:
			dprintk("%s,%d: (%d) DES CBC \n", __FILE__, __LINE__, count);
			cesa_ocf_cur_ses->cipher_alg = cri->cri_alg;
			cesa_ocf_cur_ses->ivlen = MV_CESA_DES_BLOCK_SIZE;
			cesa_ses->cryptoAlgorithm = MV_CESA_CRYPTO_DES;
			cesa_ses->cryptoMode = MV_CESA_CRYPTO_CBC;
			if(cri->cri_klen/8 > MV_CESA_MAX_CRYPTO_KEY_LENGTH) {
				printk("%s,%d: CRYPTO key too long.\n", __FILE__, __LINE__);
				goto error;
			}
			memcpy(cesa_ses->cryptoKey, cri->cri_key, cri->cri_klen/8);
			cesa_ses->cryptoKeyLength = cri->cri_klen/8;
			encrypt += count;
			break;
                case CRYPTO_MD5:
                case CRYPTO_MD5_HMAC:
			dprintk("%s,%d: (%d) %sMD5 CBC \n", __FILE__, __LINE__, count, (cri->cri_alg != CRYPTO_MD5)? "H-":" ");
                        cesa_ocf_cur_ses->auth_alg = cri->cri_alg;
			cesa_ocf_cur_ses->digestlen = (cri->cri_alg == CRYPTO_MD5)? MV_CESA_MD5_DIGEST_SIZE : 12;
			cesa_ses->macMode = (cri->cri_alg == CRYPTO_MD5)? MV_CESA_MAC_MD5 : MV_CESA_MAC_HMAC_MD5;
			if(cri->cri_klen/8 > MV_CESA_MAX_CRYPTO_KEY_LENGTH) {
				printk("%s,%d: MAC key too long. \n", __FILE__, __LINE__);
				goto error;
			}
			cesa_ses->macKeyLength = cri->cri_klen/8;
			memcpy(cesa_ses->macKey, cri->cri_key, cri->cri_klen/8);
			cesa_ses->digestSize = cesa_ocf_cur_ses->digestlen;
			auth += count;
			break;
                case CRYPTO_SHA1:
                case CRYPTO_SHA1_HMAC:
			dprintk("%s,%d: (%d) %sSHA1 CBC \n", __FILE__, __LINE__, count, (cri->cri_alg != CRYPTO_SHA1)? "H-":" ");
                        cesa_ocf_cur_ses->auth_alg = cri->cri_alg;
			cesa_ocf_cur_ses->digestlen = (cri->cri_alg == CRYPTO_SHA1)? MV_CESA_SHA1_DIGEST_SIZE : 12;
			cesa_ses->macMode = (cri->cri_alg == CRYPTO_SHA1)? MV_CESA_MAC_SHA1 : MV_CESA_MAC_HMAC_SHA1;
			if(cri->cri_klen/8 > MV_CESA_MAX_CRYPTO_KEY_LENGTH) {
				printk("%s,%d: MAC key too long. \n", __FILE__, __LINE__);
				goto error;
			}
			cesa_ses->macKeyLength = cri->cri_klen/8;
			memcpy(cesa_ses->macKey, cri->cri_key, cri->cri_klen/8);
			cesa_ses->digestSize = cesa_ocf_cur_ses->digestlen;
			auth += count;
			break;
                default:
                        printk("%s,%d: unknown algo 0x%x\n", __FILE__, __LINE__, cri->cri_alg);
                        goto error;
                }
                cri = cri->cri_next;
		count++;
        }

	if((encrypt > 2) || (auth > 2)) {
		printk("%s,%d: session mode is not supported.\n", __FILE__, __LINE__);
                goto error;
	}

	/* create new sessions in HAL */
	if(encrypt) {
		cesa_ses->operation = MV_CESA_CRYPTO_ONLY;
		/* encrypt session */
		if(auth == 1) {
			cesa_ses->operation = MV_CESA_MAC_THEN_CRYPTO;
		}
		else if(auth == 2) {
			cesa_ses->operation = MV_CESA_CRYPTO_THEN_MAC;
			cesa_ocf_cur_ses->encrypt_tn_auth = 1;
		}
		else {
			cesa_ses->operation = MV_CESA_CRYPTO_ONLY;
		}
		cesa_ses->direction = MV_CESA_DIR_ENCODE;
		spin_lock_irqsave(&cesa_lock, flags);
		status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->sid_encrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
		if(status != MV_OK) {
			printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
			goto error;
		}
		/* decrypt session */
		if( cesa_ses->operation == MV_CESA_MAC_THEN_CRYPTO ) {
			cesa_ses->operation = MV_CESA_CRYPTO_THEN_MAC;
		}
		else if( cesa_ses->operation == MV_CESA_CRYPTO_THEN_MAC ) {
			cesa_ses->operation = MV_CESA_MAC_THEN_CRYPTO;
		}
		cesa_ses->direction = MV_CESA_DIR_DECODE;
		spin_lock_irqsave(&cesa_lock, flags);
		status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->sid_decrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
		if(status != MV_OK) {
			printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
			goto error;
		}

		/* preapre one action sessions for case we will need to split an action */
#ifdef CESA_OCF_SPLIT
		if(( cesa_ses->operation == MV_CESA_MAC_THEN_CRYPTO ) ||
			( cesa_ses->operation == MV_CESA_CRYPTO_THEN_MAC )) {
			/* open one session for encode and one for decode */
			cesa_ses->operation = MV_CESA_CRYPTO_ONLY;
			cesa_ses->direction = MV_CESA_DIR_ENCODE;
			spin_lock_irqsave(&cesa_lock, flags);
			status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->frag_wa_encrypt);
			spin_unlock_irqrestore(&cesa_lock, flags);
			if(status != MV_OK) {
				printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
				goto error;
			}

			cesa_ses->direction = MV_CESA_DIR_DECODE;
			spin_lock_irqsave(&cesa_lock, flags);
			status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->frag_wa_decrypt);
			spin_unlock_irqrestore(&cesa_lock, flags);
			if(status != MV_OK) {
				printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
				goto error;
			}
			/* open one session for auth */
			cesa_ses->operation = MV_CESA_MAC_ONLY;
			cesa_ses->direction = MV_CESA_DIR_ENCODE;
			spin_lock_irqsave(&cesa_lock, flags);
			status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->frag_wa_auth);
			spin_unlock_irqrestore(&cesa_lock, flags);
			if(status != MV_OK) {
				printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
				goto error;
			}
		}
#endif
	}
	else { /* only auth */
		cesa_ses->operation = MV_CESA_MAC_ONLY;
		cesa_ses->direction = MV_CESA_DIR_ENCODE;
		spin_lock_irqsave(&cesa_lock, flags);
		status = mvCesaIfSessionOpen(cesa_ses, &cesa_ocf_cur_ses->sid_encrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
		if(status != MV_OK) {
			printk("%s,%d: Can't open new session - status = 0x%x\n", __FILE__, __LINE__, status);
			goto error;
		}
	}

	return 0;
error:
	cesa_ocf_freesession(NULL, *sid);
	return EINVAL;

}


/*
 * Free a session.
 */
static int
cesa_ocf_freesession(device_t dev, u_int64_t tid)
{
        struct cesa_ocf_data *cesa_ocf_cur_ses;
        u_int32_t sid = CRYPTO_SESID2LID(tid);
	unsigned long flags = 0;

        dprintk("%s() %d \n", __func__, sid);
	if (sid > cesa_ocf_sesnum || cesa_ocf_sessions == NULL ||
			cesa_ocf_sessions[sid] == NULL) {
		dprintk("%s,%d: EINVAL\n", __FILE__, __LINE__);
		return EINVAL;
	}

        /* Silently accept and return */
        if (sid == 0)
                return(0);

	/* release session from HAL */
	cesa_ocf_cur_ses = cesa_ocf_sessions[sid];
	if (cesa_ocf_cur_ses->sid_encrypt != -1) {
		spin_lock_irqsave(&cesa_lock, flags);
		mvCesaIfSessionClose(cesa_ocf_cur_ses->sid_encrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
	}
	if (cesa_ocf_cur_ses->sid_decrypt != -1) {
		spin_lock_irqsave(&cesa_lock, flags);
		mvCesaIfSessionClose(cesa_ocf_cur_ses->sid_decrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
	}
	if (cesa_ocf_cur_ses->frag_wa_encrypt != -1) {
		spin_lock_irqsave(&cesa_lock, flags);
		mvCesaIfSessionClose(cesa_ocf_cur_ses->frag_wa_encrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
	}
	if (cesa_ocf_cur_ses->frag_wa_decrypt != -1) {
		spin_lock_irqsave(&cesa_lock, flags);
		mvCesaIfSessionClose(cesa_ocf_cur_ses->frag_wa_decrypt);
		spin_unlock_irqrestore(&cesa_lock, flags);
	}
	if (cesa_ocf_cur_ses->frag_wa_auth != -1) {
		spin_lock_irqsave(&cesa_lock, flags);
		mvCesaIfSessionClose(cesa_ocf_cur_ses->frag_wa_auth);
		spin_unlock_irqrestore(&cesa_lock, flags);
	}

	kfree(cesa_ocf_cur_ses);
	cesa_ocf_sessions[sid] = NULL;

        return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30))
extern int crypto_init(void);
#endif

/*
 * our driver startup and shutdown routines
 */
static int
cesa_ocf_probe(struct platform_device *pdev)
{
	u8 chan = 0;
	const char *irq_str[] = {"cesa0","cesa1"};
	const char *cesa_m;
	unsigned int mask;
	struct device_node *np;
	struct clk *clk;
	int err, i, j;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "CESA device node not available\n");
		return -ENOENT;
	}

	/*
	 * Check driver mode from dts
	 */
	cesa_m = of_get_property(pdev->dev.of_node, "cesa,mode", NULL);
	if (strncmp(cesa_m, "ocf", 3) != 0) {
		dprintk("%s: device operate in %s mode\n", __func__, cesa_m);
		return -ENODEV;
	}
	mv_cesa_mode = CESA_OCF_M;

	err = mv_get_cesa_resources(pdev);
	if (err != 0)
		return err;

	j = of_property_count_strings(pdev->dev.of_node, "clock-names");
	dprintk("%s: Gate %d clocks\n", __func__, (j > 0 ? j : 1));
	/*
	 * If property "clock-names" does not exist (j < 0), assume that there
	 * is only one clock which needs gating (j > 0 ? j : 1)
	 */
	for (i = 0; i < (j > 0 ? j : 1); i++) {

		/* Not all platforms can gate the clock, so it is not
		 * an error if the clock does not exists.
		 */
		clk = of_clk_get(pdev->dev.of_node, i);
		if (!IS_ERR(clk))
			clk_prepare_enable(clk);
	}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30))
	crypto_init();
#endif

	/* Init globals */
	next_result = 0;
	result_done = 0;

	/* The pool size here is twice bigger than requests queue size due to possible reordering */
	cesa_ocf_pool = (struct cesa_ocf_process*)kmalloc((sizeof(struct cesa_ocf_process) *
					CESA_OCF_POOL_SIZE), GFP_KERNEL);
	if (cesa_ocf_pool == NULL) {
		dev_err(&pdev->dev, "%s,%d: ENOBUFS\n", __FILE__, __LINE__);
		return -EINVAL;
	}

	for (cesa_ocf_stack_idx = 0; cesa_ocf_stack_idx < CESA_OCF_POOL_SIZE; cesa_ocf_stack_idx++)
		cesa_ocf_stack[cesa_ocf_stack_idx] = &cesa_ocf_pool[cesa_ocf_stack_idx];

	memset(cesa_ocf_pool, 0, (sizeof(struct cesa_ocf_process) * CESA_OCF_POOL_SIZE));
	memset(&mv_cesa_dev, 0, sizeof(mv_cesa_dev));
	softc_device_init(&mv_cesa_dev, "MV CESA", 0, mv_cesa_methods);
	cesa_ocf_id = crypto_get_driverid(softc_get_device(&mv_cesa_dev),CRYPTOCAP_F_HARDWARE);

	if (cesa_ocf_id < 0)
		panic("MV CESA crypto device cannot initialize!");

	dprintk("%s,%d: cesa ocf device id is %d\n",
					      __FILE__, __LINE__, cesa_ocf_id);

	if (MV_OK !=
	    mvSysCesaInit(CESA_OCF_MAX_SES*5, CESA_Q_SIZE, &pdev->dev, pdev)) {
		dev_err(&pdev->dev, "%s,%d: mvCesaInit Failed.\n",
							   __FILE__, __LINE__);
		return -EINVAL;
	}

	if (mv_cesa_feature == INT_COALESCING)
		mask = MV_CESA_CAUSE_EOP_COAL_MASK;
	else
		mask = MV_CESA_CAUSE_ACC_DMA_MASK;

	/*
	 * Preparation for each CESA chan
	 */
	for_each_child_of_node(pdev->dev.of_node, np) {
		int irq;

		/*
		 * Get IRQ from FDT and map it to the Linux IRQ nr
		 */
		irq = irq_of_parse_and_map(np, 0);
		if (!irq) {
			dev_err(&pdev->dev, "IRQ nr missing in device tree\n");
			return -ENOENT;
		}

		dprintk("%s: cesa irq %d, chan %d\n", __func__,
					      irq, chan);

		/* clear and unmask Int */
		MV_REG_WRITE( MV_CESA_ISR_CAUSE_REG(chan), 0);
		MV_REG_WRITE( MV_CESA_ISR_MASK_REG(chan), mask);

		chan_id[chan] = chan;

		/* register interrupt */
		if (request_irq(irq, cesa_interrupt_handler,
				(IRQF_DISABLED) , irq_str[chan], &chan_id[chan]) < 0) {
			dev_err(&pdev->dev, "%s,%d: cannot assign irq %x\n",
			    __FILE__, __LINE__, irq);
			return -EINVAL;
		}

		chan++;
	}

#ifdef CESA_OCF_TASKLET
	tasklet_init(&cesa_ocf_tasklet, cesa_callback, (unsigned int) 0);
#endif

#define	REGISTER(alg) \
	crypto_register(cesa_ocf_id, alg, 0,0)
	REGISTER(CRYPTO_AES_CBC);
	REGISTER(CRYPTO_DES_CBC);
	REGISTER(CRYPTO_3DES_CBC);
	REGISTER(CRYPTO_MD5);
	REGISTER(CRYPTO_MD5_HMAC);
	REGISTER(CRYPTO_SHA1);
	REGISTER(CRYPTO_SHA1_HMAC);
#undef REGISTER

#ifdef CESA_DEBUGS
	mvCesaDebugRegs();
#endif
	dev_info(&pdev->dev, "%s: CESA driver operate in %s(%d) mode\n",
					       __func__, cesa_m, mv_cesa_mode);
	return 0;
}

static int
cesa_ocf_remove(struct platform_device *pdev)
{
	struct device_node *np;
	u8 chan = 0;
	int irq;

	dprintk("%s()\n", __func__);

	crypto_unregister_all(cesa_ocf_id);
	cesa_ocf_id = -1;
	kfree(cesa_ocf_pool);

	for_each_child_of_node(pdev->dev.of_node, np) {

		irq = irq_of_parse_and_map(np, 0);
		if (!irq) {
			dev_err(&pdev->dev, "IRQ nr missing in device tree\n");
			return -ENOENT;
		}

		free_irq(irq, NULL);

		/* mask and clear Int */
		MV_REG_WRITE( MV_CESA_ISR_MASK_REG(chan), 0);
		MV_REG_WRITE( MV_CESA_ISR_CAUSE_REG(chan), 0);

		chan++;
	}

	if( MV_OK != mvCesaIfFinish() ) {
		dev_err(&pdev->dev, "%s,%d: mvCesaFinish Failed.\n",
							   __FILE__, __LINE__);
		return -EINVAL;
	}
	return 0;
}

void cesa_ocf_shutdown(struct platform_device *pdev)
{
	struct clk *clk;
	int  i, j;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "CESA device node not available\n");
		return;
	}

	j = of_property_count_strings(pdev->dev.of_node, "clock-names");
	dprintk("%s: Gate %d clocks\n", __func__, (j > 0 ? j : 1));
	/*
	 * If property "clock-names" does not exist (j < 0), assume that there
	 * is only one clock which needs gating (j > 0 ? j : 1)
	 */
	for (i = 0; i < (j > 0 ? j : 1); i++) {
		/* Not all platforms can gate the clock, so it is not
		 * an error if the clock does not exists.
		 */
		clk = of_clk_get(pdev->dev.of_node, i);
		if (!IS_ERR(clk))
			clk_disable_unprepare(clk);
	}
}

static struct of_device_id mv_cesa_dt_ids[] = {
	{ .compatible = "marvell,armada-cesa", },
	{},
};
MODULE_DEVICE_TABLE(of, mv_cesa_dt_ids);

static struct platform_driver mv_cesa_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(mv_cesa_dt_ids),
	},
	.probe		= cesa_ocf_probe,
	.remove		= cesa_ocf_remove,
	.shutdown	= cesa_ocf_shutdown,
#ifdef CONFIG_PM
	.resume		= cesa_resume,
	.suspend	= cesa_suspend,
#endif
};

static int __init cesa_ocf_init(void)
{
	return platform_driver_register(&mv_cesa_driver);
}
module_init(cesa_ocf_init);

static void __exit cesa_ocf_exit(void)
{
	platform_driver_unregister(&mv_cesa_driver);
}
module_exit(cesa_ocf_exit);

MODULE_LICENSE("Marvell/GPL");
MODULE_AUTHOR("Ronen Shitrit");
MODULE_DESCRIPTION("OCF module for Marvell CESA based SoC");
