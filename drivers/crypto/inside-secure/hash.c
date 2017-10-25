/*
 * Copyright (C) 2016 Marvell
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <crypto/sha.h>
#include <linux/dmapool.h>

#include "safexcel.h"

struct safexcel_ahash_ctx {
	struct safexcel_context base;
	struct safexcel_crypto_priv *priv;

	u32 alg;
	u32 digest;

	u32 ipad[SHA1_DIGEST_SIZE / sizeof(u32)];
	u32 opad[SHA1_DIGEST_SIZE / sizeof(u32)];
};

static const u8 sha1_zero_digest[SHA1_DIGEST_SIZE] = {
	0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
	0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09,
};

static const u8 sha224_zero_digest[SHA224_DIGEST_SIZE] = {
	0xd1, 0x4a, 0x02, 0x8c, 0x2a, 0x3a, 0x2b, 0xc9, 0x47,
	0x61, 0x02, 0xbb, 0x28, 0x82, 0x34, 0xc4, 0x15, 0xa2,
	0xb0, 0x1f, 0x82, 0x8e, 0xa6, 0x2a, 0xc5, 0xb3, 0xe4,
	0x2f
};

static const u8 sha256_zero_digest[SHA256_DIGEST_SIZE] = {
	0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a,
	0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae,
	0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99,
	0x1b, 0x78, 0x52, 0xb8, 0x55
};

/* Build hash token */
static void safexcel_hash_token(struct safexcel_command_desc *cdesc,
				u32 input_length, u32 result_length)
{
	struct safexcel_token *token =
		(struct safexcel_token *)cdesc->control_data.token;

	token[0].opcode = EIP197_TOKEN_OPCODE_DIRECTION;
	token[0].packet_length = input_length;
	token[0].stat = EIP197_TOKEN_STAT_LAST_HASH;
	token[0].instructions = EIP197_TOKEN_INS_TYPE_HASH;

	token[1].opcode = EIP197_TOKEN_OPCODE_INSERT;
	token[1].packet_length = result_length;
	token[1].stat = EIP197_TOKEN_STAT_LAST_HASH |
			EIP197_TOKEN_STAT_LAST_PACKET;
	token[1].instructions = EIP197_TOKEN_INS_TYPE_OUTPUT |
				EIP197_TOKEN_INS_INSERT_HASH_DIGEST;
}

/* Build hash context control data */
static void safexcel_context_control(struct safexcel_ahash_ctx *ctx,
				     struct safexcel_ahash_req *req,
				     struct safexcel_command_desc *cdesc,
				     unsigned int digestsize,
				     unsigned int blocksize)
{
	int i;

	cdesc->control_data.control0 |= CONTEXT_CONTROL_TYPE_HASH_OUT;
	cdesc->control_data.control0 |= ctx->alg;
	cdesc->control_data.control0 |= ctx->digest;

	if (ctx->digest == CONTEXT_CONTROL_DIGEST_PRECOMPUTED) {
		if (req->len) {
			if (ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA1)
				cdesc->control_data.control0 |= CONTEXT_CONTROL_SIZE(6);
			else if (ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA224 ||
				 ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA256)
				cdesc->control_data.control0 |= CONTEXT_CONTROL_SIZE(9);

			cdesc->control_data.control1 |= CONTEXT_CONTROL_DIGEST_CNT;

		} else {
			cdesc->control_data.control0 |= CONTEXT_CONTROL_RESTART_HASH;
		}

		if (!req->finish)
			cdesc->control_data.control0 |= CONTEXT_CONTROL_NO_FINISH_HASH;

		/*
		 * Copy the input digest if needed, and setup the context
		 * fields. Do this now as we need it to setup the first command
		 * descriptor.
		 */
		if (req->len) {
			for (i = 0; i < digestsize / sizeof(u32); i++)
				ctx->base.ctxr->data[i] = cpu_to_le32(req->state[i]);

			if (req->finish)
				ctx->base.ctxr->data[i] = cpu_to_le32(req->len / blocksize);
		}
	} else if (ctx->digest == CONTEXT_CONTROL_DIGEST_HMAC) {
		cdesc->control_data.control0 |= CONTEXT_CONTROL_SIZE(10);

		memcpy(ctx->base.ctxr->data, ctx->ipad, digestsize);
		memcpy(ctx->base.ctxr->data + 5, ctx->opad, digestsize);
	}
}

/* Handle a hash result descriptor */
static int safexcel_handle_req_result(struct safexcel_crypto_priv *priv, int ring,
				      struct crypto_async_request *async,
				      bool *should_complete, int *ret)
{
	struct safexcel_result_desc *rdesc;
	int cache_next_len, len;
	struct ahash_request *areq = ahash_request_cast(async);
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_ahash_req *sreq = ahash_request_ctx(areq);
	int result_sz = sreq->state_sz;

	*ret = 0;

	spin_lock_bh(&priv->ring[ring].egress_lock);
	rdesc = safexcel_ring_next_rptr(priv, &priv->ring[ring].rdr);
	if (IS_ERR(rdesc)) {
		dev_err(priv->dev,
			"hash: result: could not retrieve the result descriptor\n");
		*ret = PTR_ERR(rdesc);
	} else if (rdesc->result_data.error_code) {
		dev_err(priv->dev,
			"hash: result: result descriptor error (%d)\n",
			rdesc->result_data.error_code);
		*ret = -EINVAL;
	}

	safexcel_complete(priv, ring);
	spin_unlock_bh(&priv->ring[ring].egress_lock);

	if (sreq->finish)
		result_sz = crypto_ahash_digestsize(ahash);
	memcpy(sreq->state, areq->result, result_sz);

	dma_unmap_sg(priv->dev, areq->src,
		     sg_nents_for_len(areq->src, areq->nbytes), DMA_TO_DEVICE);

	safexcel_free_context(priv, async, sreq->state_sz);

	len = sreq->len;
	cache_next_len = do_div(len, crypto_ahash_blocksize(ahash));
	if (cache_next_len)
		memcpy(sreq->cache, sreq->cache_next, cache_next_len);

	*should_complete = true;

	return 1;
}

/* Send hash command to the engine */
static int safexcel_ahash_send_req(struct crypto_async_request *async, int ring,
				    struct safexcel_request *request, int *commands,
				    int *results)
{
	struct ahash_request *areq = ahash_request_cast(async);
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_crypto_priv *priv = ctx->priv;
	struct safexcel_command_desc *cdesc, *first_cdesc = NULL;
	struct safexcel_result_desc *rdesc;
	struct scatterlist *sg;
	int i, nents, queued, len = req->len, n_cdesc = 0, ret = 0;
	int cache_len = do_div(len, crypto_ahash_blocksize(ahash));

	if (req->last_req) {
		if (!cache_len)
			queued = len = areq->nbytes;
		else
			queued = len = cache_len;
	} else {
		queued = len = (cache_len + areq->nbytes) &
			       ~(crypto_ahash_blocksize(ahash) - 1);
	}

	spin_lock_bh(&priv->ring[ring].egress_lock);

	/* Add a command descriptor for the cached data, if any */
	if (cache_len) {
		ctx->base.cache_dma = dma_map_single(priv->dev, req->cache,
						      cache_len, DMA_TO_DEVICE);
		if (dma_mapping_error(priv->dev, ctx->base.cache_dma)) {
			ret = -EINVAL;
			goto unlock;
		}

		ctx->base.cache_sz = cache_len;
		first_cdesc = safexcel_add_cdesc(priv, ring, 1,
						 (cache_len == len),
						 ctx->base.cache_dma,
						 cache_len, len,
						 ctx->base.ctxr_dma);
		if (IS_ERR(first_cdesc)) {
			ret = PTR_ERR(first_cdesc);
			goto free_cache;
		}
		n_cdesc++;

		queued -= cache_len;
		if (!queued)
			goto send_command;
	}

	/* Now handle the current ahash request buffer(s) */
	nents = sg_nents_for_len(areq->src, areq->nbytes);
	if (dma_map_sg(priv->dev, areq->src, nents, DMA_TO_DEVICE) <= 0) {
		ret = -ENOMEM;
		goto cdesc_rollback;
	}

	for_each_sg(areq->src, sg, nents, i) {
		int sglen = sg_dma_len(sg);

		/* Do not overflow the request */
		if (queued - sglen < 0)
			sglen = queued;

		cdesc = safexcel_add_cdesc(priv, ring, !n_cdesc,
					   !(queued - sglen), sg_dma_address(sg),
					   sglen, len, ctx->base.ctxr_dma);

		if (IS_ERR(cdesc)) {
			ret = PTR_ERR(cdesc);
			goto cdesc_rollback;
		}
		n_cdesc++;

		if (n_cdesc == 1)
			first_cdesc = cdesc;

		queued -= sglen;
		if (!queued)
			break;
	}

send_command:
	/* Setup the context options */
	safexcel_context_control(ctx, req, first_cdesc,
				 req->state_sz,
				 crypto_ahash_blocksize(ahash));

	/* Add the token */
	safexcel_hash_token(first_cdesc, len, req->state_sz);

	ctx->base.result_dma = dma_map_single(priv->dev, areq->result,
					      req->state_sz,
					      DMA_FROM_DEVICE);
	if (dma_mapping_error(priv->dev, ctx->base.result_dma)) {
		ret = -EINVAL;
		goto cdesc_rollback;
	}

	/* Add a result descriptor */
	rdesc = safexcel_add_rdesc(priv, ring, 1, 1,
				   ctx->base.result_dma,
				   req->state_sz);
	if (IS_ERR(rdesc)) {
		ret = PTR_ERR(rdesc);
		goto cdesc_rollback;
	}

	request->req = &areq->base;

	list_add_tail(&request->list, &priv->ring[ring].list);

	spin_unlock_bh(&priv->ring[ring].egress_lock);

	req->len += areq->nbytes;

	*commands = n_cdesc;
	*results = 1;
	return 0;

cdesc_rollback:
	if (nents)
		dma_unmap_sg(priv->dev, areq->src,
			     sg_nents_for_len(areq->src, areq->nbytes),
			     DMA_TO_DEVICE);

	for (i = 0; i < n_cdesc; i++)
		safexcel_ring_rollback_wptr(priv, &priv->ring[ring].cdr);
free_cache:
	if (ctx->base.cache_dma)
		dma_unmap_single(priv->dev, ctx->base.cache_dma,
				 ctx->base.cache_sz, DMA_TO_DEVICE);

unlock:
	spin_unlock_bh(&priv->ring[ring].egress_lock);
	return ret;
}

/* Check if the request needs invalidation (context was changed) */
static inline bool safexcel_ahash_needs_inv_get(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	unsigned int state_w_sz = req->state_sz / sizeof(u32);
	int i;

	for (i = 0; i < state_w_sz; i++)
		if (ctx->base.ctxr->data[i] != cpu_to_le32(req->state[i]))
			return true;

	if (ctx->base.ctxr->data[state_w_sz] !=
	    cpu_to_le32(req->len / crypto_ahash_blocksize(ahash)))
		return true;

	return false;
}

/* Handle a hash invalidation descriptor */
static int safexcel_handle_inv_result(struct safexcel_crypto_priv *priv,
				      int ring,
				      struct crypto_async_request *async,
				      bool *should_complete, int *ret)
{
	struct safexcel_result_desc *rdesc;
	struct ahash_request *areq = ahash_request_cast(async);
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(ahash);
	int enq_ret;

	*ret = 0;

	spin_lock_bh(&priv->ring[ring].egress_lock);
	rdesc = safexcel_ring_next_rptr(priv, &priv->ring[ring].rdr);
	if (IS_ERR(rdesc)) {
		dev_err(priv->dev,
			"hash: invalidate: could not retrieve the result descriptor\n");
		*ret = PTR_ERR(rdesc);
	} else if (rdesc->result_data.error_code) {
		dev_err(priv->dev,
			"hash: invalidate: result descriptor error (%d)\n",
			rdesc->result_data.error_code);
		*ret = -EINVAL;
	}

	safexcel_complete(priv, ring);
	spin_unlock_bh(&priv->ring[ring].egress_lock);

	if (ctx->base.exit_inv) {
		dma_pool_free(priv->context_pool, ctx->base.ctxr,
			      ctx->base.ctxr_dma);

		*should_complete = true;

		return 1;
	}

	ctx->base.ring = safexcel_select_ring(priv);
	ring = ctx->base.ring;

	spin_lock_bh(&priv->ring[ring].queue_lock);
	enq_ret = ahash_enqueue_request(&priv->ring[ring].queue, areq);
	spin_unlock_bh(&priv->ring[ring].queue_lock);

	if (enq_ret != -EINPROGRESS)
		*ret = enq_ret;

	queue_work(priv->ring[ring].workqueue,
		   &priv->ring[ring].work_data.work);

	*should_complete = false;

	return 1;
}

static int safexcel_handle_result(struct safexcel_crypto_priv *priv, int ring,
				  struct crypto_async_request *async,
				  bool *should_complete, int *ret)
{
	struct ahash_request *areq = ahash_request_cast(async);
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	int err;

	if (req->needs_inv) {
		req->needs_inv = false;
		err = safexcel_handle_inv_result(priv, ring, async,
						 should_complete, ret);
	} else
		err = safexcel_handle_req_result(priv, ring, async,
						 should_complete, ret);

	return err;
}

/* Send hash invalidation command to the engine */
static int safexcel_ahash_send_inv(struct crypto_async_request *async,
				   int ring, struct safexcel_request *request,
				   int *commands, int *results)
{
	struct ahash_request *areq = ahash_request_cast(async);
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	int ret;

	ret = safexcel_invalidate_cache(async, &ctx->base, ctx->priv,
					ctx->base.ctxr_dma, ring, request);
	if (unlikely(ret))
		return ret;

	*commands = 1;
	*results = 1;

	return 0;
}

static int safexcel_ahash_send(struct crypto_async_request *async,
			 int ring, struct safexcel_request *request,
			 int *commands, int *results)
{

	struct ahash_request *areq = ahash_request_cast(async);
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	int ret;

	if (req->needs_inv)
		ret = safexcel_ahash_send_inv(async, ring, request,
					      commands, results);
	else
		ret = safexcel_ahash_send_req(async, ring, request,
					      commands, results);
	return ret;
}

/* Upon context exit, send invalidation command */
static int safexcel_ahash_exit_inv(struct crypto_tfm *tfm)
{
	struct safexcel_ahash_ctx *ctx = crypto_tfm_ctx(tfm);
	struct safexcel_crypto_priv *priv = ctx->priv;
	AHASH_REQUEST_ON_STACK(req, __crypto_ahash_cast(tfm));
	struct safexcel_ahash_req *sreq = ahash_request_ctx(req);
	struct safexcel_inv_result result = { 0 };
	int ring = ctx->base.ring;
	int ret;

	memset(req, 0, sizeof(struct ahash_request));

	/* create invalidation request */
	init_completion(&result.completion);
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   safexcel_inv_complete, &result);

	ahash_request_set_tfm(req, __crypto_ahash_cast(tfm));
	ctx = crypto_tfm_ctx(req->base.tfm);
	ctx->base.exit_inv = true;
	sreq->needs_inv = true;

	spin_lock_bh(&priv->ring[ring].queue_lock);
	ret = ahash_enqueue_request(&priv->ring[ring].queue, req);
	spin_unlock_bh(&priv->ring[ring].queue_lock);

	queue_work(priv->ring[ring].workqueue,
		   &priv->ring[ring].work_data.work);

	wait_for_completion_interruptible(&result.completion);

	if (result.error) {
		dev_warn(priv->dev, "hash: completion error (%d)\n",
			 result.error);
		return result.error;
	}

	return ret;
}

static int safexcel_ahash_update(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_crypto_priv *priv = ctx->priv;
	int queued, len = req->len, ret, ring;
	int cache_len = do_div(len, crypto_ahash_blocksize(ahash));

	/*
	 * We're not doing partial updates when performing an hmac request, and
	 * we're not using the cache. Everything will be handled by the final()
	 * call.
	 */
	if (req->hmac && !req->last_req)
		return 0;

	if (req->last_req) {
		if (!cache_len)
			queued = len = areq->nbytes;
		else
			queued = len = cache_len;
	} else {
		queued = len = cache_len + areq->nbytes;
	}

	/* If the request is 0 length and is not the last request, do nothing */
	if (!areq->nbytes && !req->last_req)
		return 0;

	/*
	 * If we have an overall 0 length request, we only need a "dummy"
	 * command descriptor.
	 */
	if (!queued) {
		if (ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA1)
			memcpy(areq->result, sha1_zero_digest, SHA1_DIGEST_SIZE);
		else if (ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA224)
			memcpy(areq->result, sha224_zero_digest, SHA224_DIGEST_SIZE);
		else if (ctx->alg == CONTEXT_CONTROL_CRYPTO_ALG_SHA256)
			memcpy(areq->result, sha256_zero_digest, SHA256_DIGEST_SIZE);

		return 0;
	}

	/* If this isn't the last request, we can involve the cache */
	if (!req->last_req) {
		u32 extra;

		/*
		 * In case there isn't enough bytes to proceed (less than a
		 * block size + 1), cache the data until we have enough.
		 *
		 * The last request should have the CONTEXT_CONTROL_FINISH_HASH
		 * bit set in the first context control word. This means we
		 * can't cache a request of exactly one block size because we
		 * wouldn't be able to perform this when called from final():
		 * the request size would be 0, which isn't supported.
		 */
		if (cache_len + areq->nbytes < crypto_ahash_blocksize(ahash) + 1) {
			sg_pcopy_to_buffer(areq->src, sg_nents(areq->src),
					   req->cache + cache_len,
					   areq->nbytes, 0);
			return 0;
		}

		/*
		 * In case we have enough data to proceed, we can send n blocks
		 * to the hash engine while caching the bytes not in these
		 * blocks.
		 */
		extra = len & (crypto_ahash_blocksize(ahash) - 1);

		/*
		 * For the exact same reason as stated above, if we have a
		 * request size being exactly a multiple of the block size, we
		 * must queue one block in case it's the final one.
		 */
		if (!extra)
			extra = crypto_ahash_blocksize(ahash);


		sg_pcopy_to_buffer(areq->src, sg_nents(areq->src),
				   req->cache_next, extra,
				   areq->nbytes - extra);
		len -= extra;
		queued -= extra;
	}

	req->needs_inv = false;

	/*
	 * Check if the context exists, if yes:
	 *	- EIP197: check if it needs to be invalidated
	 *	- EIP97: Nothing to be done
	 * If context not exists, allocate it (for both EIP97 & EIP197)
	 * and set the send routine for the new allocated context.
	 * If it's EIP97 with existing context, the send routine is already set.
	 */
	if (ctx->base.ctxr) {
		if (priv->eip_type == EIP197 && ctx->base.needs_inv) {
			ctx->base.needs_inv = false;
			req->needs_inv = true;
		}
	} else {
		ctx->base.ring = safexcel_select_ring(priv);
		ctx->base.ctxr = dma_pool_zalloc(priv->context_pool,
						 EIP197_GFP_FLAGS(areq->base),
						 &ctx->base.ctxr_dma);
		if (!ctx->base.ctxr)
			return -ENOMEM;
	}

	ring = ctx->base.ring;

	spin_lock_bh(&priv->ring[ring].queue_lock);
	ret = ahash_enqueue_request(&priv->ring[ring].queue, areq);
	spin_unlock_bh(&priv->ring[ring].queue_lock);

	queue_work(priv->ring[ring].workqueue,
		   &priv->ring[ring].work_data.work);

	return ret;
}

static int safexcel_ahash_final(struct ahash_request *areq)
{
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_crypto_priv *priv = ctx->priv;

	req->last_req = true;
	req->finish = true;

	/*
	 * Check if we need to invalidate the context,
	 * this should be done only for EIP197 (no cache in EIP97).
	 */
	if (priv->eip_type == EIP197 && req->len && ctx->base.ctxr &&
	    ctx->digest == CONTEXT_CONTROL_DIGEST_PRECOMPUTED)
		ctx->base.needs_inv = safexcel_ahash_needs_inv_get(areq);

	return safexcel_ahash_update(areq);
}

static int safexcel_ahash_finup(struct ahash_request *areq)
{
	return safexcel_ahash_final(areq);
}

static int safexcel_sha1_export(struct ahash_request *areq, void *out)
{
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct sha1_state *sha1 = out;
	int len = req->len;
	int cache_len = do_div(len, crypto_ahash_blocksize(ahash));

	sha1->count = req->len;
	memcpy(sha1->state, req->state, req->state_sz);
	memset(sha1->buffer, 0, crypto_ahash_blocksize(ahash));
	memcpy(sha1->buffer, req->cache, cache_len);

	return 0;
}

static int safexcel_sha1_import(struct ahash_request *areq, const void *in)
{
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	const struct sha1_state *sha1 = in;

	memset(req, 0, sizeof(*req));

	req->len = sha1->count;
	memcpy(req->cache, sha1->buffer, sha1->count);
	memcpy(req->state, sha1->state, req->state_sz);

	return 0;
}

static int safexcel_ahash_cra_init(struct crypto_tfm *tfm)
{
	struct safexcel_ahash_ctx *ctx = crypto_tfm_ctx(tfm);
	struct safexcel_alg_template *tmpl =
		container_of(__crypto_ahash_alg(tfm->__crt_alg),
			     struct safexcel_alg_template, alg.ahash);

	ctx->priv = tmpl->priv;
	ctx->base.send = safexcel_ahash_send;
	ctx->base.handle_result = safexcel_handle_result;

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(struct safexcel_ahash_req));
	return 0;
}

static int safexcel_sha1_init(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);

	memset(req, 0, sizeof(*req));

	req->state[0] = SHA1_H0;
	req->state[1] = SHA1_H1;
	req->state[2] = SHA1_H2;
	req->state[3] = SHA1_H3;
	req->state[4] = SHA1_H4;

	ctx->alg = CONTEXT_CONTROL_CRYPTO_ALG_SHA1;
	ctx->digest = CONTEXT_CONTROL_DIGEST_PRECOMPUTED;
	req->state_sz = SHA1_DIGEST_SIZE;

	return 0;
}

static int safexcel_sha1_digest(struct ahash_request *areq)
{
	int ret = safexcel_sha1_init(areq);

	if (ret)
		return ret;

	return safexcel_ahash_finup(areq);
}

static void safexcel_ahash_cra_exit(struct crypto_tfm *tfm)
{
	struct safexcel_ahash_ctx *ctx = crypto_tfm_ctx(tfm);
	struct safexcel_crypto_priv *priv = ctx->priv;
	int ret;

	/* context not allocated, skip invalidation */
	if (!ctx->base.ctxr)
		return;

	/*
	 * EIP197 has internal cache which needs to be invalidated
	 * when the context is closed.
	 * dma_pool_free will be called in the invalidation result
	 * handler (different context).
	 * EIP97 doesn't have internal cache, so no need to invalidate
	 * it and we can just release the dma pool.
	 */
	if (priv->eip_type == EIP197) {
		ret = safexcel_ahash_exit_inv(tfm);
		if (ret != -EINPROGRESS)
			dev_warn(priv->dev, "hash: invalidation error %d\n", ret);
	} else {
		dma_pool_free(priv->context_pool, ctx->base.ctxr,
			      ctx->base.ctxr_dma);
	}
}

struct safexcel_alg_template safexcel_alg_sha1 = {
	.type = SAFEXCEL_ALG_TYPE_AHASH,
	.alg.ahash = {
		.init = safexcel_sha1_init,
		.update = safexcel_ahash_update,
		.final = safexcel_ahash_final,
		.finup = safexcel_ahash_finup,
		.digest = safexcel_sha1_digest,
		.export = safexcel_sha1_export,
		.import = safexcel_sha1_import,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize = sizeof(struct sha1_state),
			.base = {
				.cra_name = "sha1",
				.cra_driver_name = "safexcel-sha1",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC |
					     CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize = SHA1_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct safexcel_ahash_ctx),
				.cra_init = safexcel_ahash_cra_init,
				.cra_exit = safexcel_ahash_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
};

static int safexcel_hmac_sha1_init(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);

	safexcel_sha1_init(areq);
	ctx->digest = CONTEXT_CONTROL_DIGEST_HMAC;
	req->hmac = true;
	return 0;
}

static int safexcel_hmac_sha1_digest(struct ahash_request *areq)
{
	int ret = safexcel_hmac_sha1_init(areq);

	if (ret)
		return ret;

	return safexcel_ahash_finup(areq);
}

struct safexcel_ahash_result {
	struct completion completion;
	int error;
};

static void safexcel_ahash_complete(struct crypto_async_request *req, int error)
{
	struct safexcel_ahash_result *result = req->data;

	if (error == -EINPROGRESS)
		return;

	result->error = error;
	complete(&result->completion);
}

static int safexcel_hmac_prepare_pad(struct ahash_request *areq, u8 *pad,
				     unsigned int blocksize, void *state,
				     bool finish)
{
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct scatterlist sg;
	struct safexcel_ahash_result result;
	int ret;

	sg_init_one(&sg, pad, blocksize);
	init_completion(&result.completion);
	ahash_request_set_crypt(areq, &sg, pad, blocksize);
	ahash_request_set_callback(areq, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   safexcel_ahash_complete, &result);

	ret = safexcel_sha1_init(areq);
	if (ret)
		return ret;

	req->last_req = 1;
	req->finish = finish;
	ret = safexcel_ahash_update(areq);
	if (ret && ret != -EINPROGRESS && ret != -EBUSY)
		return ret;

	wait_for_completion_interruptible(&result.completion);
	if (result.error)
		return result.error;

	ret = crypto_ahash_export(areq, state);
	if (ret)
		return ret;

	return 0;
}

static int safexcel_hmac_sha1_setkey(struct crypto_ahash *tfm, const u8 *key,
				     unsigned int keylen)
{
	struct safexcel_ahash_ctx *ctx = crypto_tfm_ctx(crypto_ahash_tfm(tfm));
	struct safexcel_crypto_priv *priv = ctx->priv;
	struct ahash_request *areq;
	struct crypto_ahash *ahash;
	struct sha1_state s0, s1;
	unsigned int blocksize;
	int i, ret = 0;
	u8 ipad[SHA1_BLOCK_SIZE], opad[SHA1_BLOCK_SIZE];

	/*
	 * Prepare the ipad and the opad, which are needed as input digests for
	 * the hmac operation.
	 */
	ahash = crypto_alloc_ahash("safexcel-sha1", CRYPTO_ALG_TYPE_AHASH,
				   CRYPTO_ALG_TYPE_AHASH_MASK);
	if (IS_ERR(ahash))
		return PTR_ERR(ahash);

	areq = ahash_request_alloc(ahash, GFP_KERNEL);
	if (!areq) {
		ret = -ENOMEM;
		goto free_ahash;
	}

	crypto_ahash_clear_flags(ahash, ~0);
	blocksize = crypto_tfm_alg_blocksize(crypto_ahash_tfm(ahash));

	if (keylen <= blocksize) {
		memcpy(ipad, key, keylen);
	} else if (keylen > blocksize) {
		/*
		 * If the key is larger than a block size, we need to hash the
		 * key before computing ipad and opad.
		 */
		struct sha1_state skey;
		u8 *keydup = kmemdup(key, keylen, GFP_KERNEL);

		if (!keydup) {
			ret = -ENOMEM;
			goto free_req;
		}

		ret = safexcel_hmac_prepare_pad(areq, keydup, keylen, &skey, true);
		memset(keydup, 0, keylen);
		kfree(keydup);
		if (ret)
			goto free_req;

		memcpy(ipad, skey.state, ARRAY_SIZE(skey.state) * sizeof(u32));
		keylen = ARRAY_SIZE(skey.state) * sizeof(u32);
	}
	memset(ipad + keylen, 0, blocksize - keylen);
	memcpy(opad, ipad, blocksize);

	for (i = 0; i < blocksize; i++) {
		ipad[i] ^= 0x36;
		opad[i] ^= 0x5c;
	}

	ret = safexcel_hmac_prepare_pad(areq, ipad, blocksize, &s0, false);
	if (ret)
		goto free_req;

	ret = safexcel_hmac_prepare_pad(areq, opad, blocksize, &s1, false);

	/*
	 * For EIP197 we need to Check if the ipad/opad were changed,
	 * if yes, need to invalidate the context.
	 */
	if (priv->eip_type == EIP197 && ctx->base.ctxr) {
		for (i = 0; i < ARRAY_SIZE(s0.state); i++) {
			if (ctx->ipad[i] != le32_to_cpu(s0.state[i]) ||
			    ctx->opad[i] != le32_to_cpu(s1.state[i])) {
				ctx->base.needs_inv = true;
				break;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(s0.state); i++)
		ctx->ipad[i] = le32_to_cpu(s0.state[i]);
	for (i = 0; i < ARRAY_SIZE(s1.state); i++)
		ctx->opad[i] = le32_to_cpu(s1.state[i]);

free_req:
	ahash_request_free(areq);
free_ahash:
	crypto_free_ahash(ahash);
	return ret;
}

struct safexcel_alg_template safexcel_alg_hmac_sha1 = {
	.type = SAFEXCEL_ALG_TYPE_AHASH,
	.alg.ahash = {
		.init = safexcel_hmac_sha1_init,
		.update = safexcel_ahash_update,
		.final = safexcel_ahash_final,
		.finup = safexcel_ahash_finup,
		.digest = safexcel_hmac_sha1_digest,
		.setkey = safexcel_hmac_sha1_setkey,
		.export = safexcel_sha1_export,
		.import = safexcel_sha1_import,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize = sizeof(struct sha1_state),
			.base = {
				.cra_name = "hmac(sha1)",
				.cra_driver_name = "safexcel-hmac-sha1",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC |
					     CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize = SHA1_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct safexcel_ahash_ctx),
				.cra_init = safexcel_ahash_cra_init,
				.cra_exit = safexcel_ahash_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
};

static int safexcel_sha256_init(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);

	memset(req, 0, sizeof(*req));

	req->state[0] = SHA256_H0;
	req->state[1] = SHA256_H1;
	req->state[2] = SHA256_H2;
	req->state[3] = SHA256_H3;
	req->state[4] = SHA256_H4;
	req->state[5] = SHA256_H5;
	req->state[6] = SHA256_H6;
	req->state[7] = SHA256_H7;

	ctx->alg = CONTEXT_CONTROL_CRYPTO_ALG_SHA256;
	ctx->digest = CONTEXT_CONTROL_DIGEST_PRECOMPUTED;
	req->state_sz = SHA256_DIGEST_SIZE;

	return 0;
}

static int safexcel_sha256_digest(struct ahash_request *areq)
{
	int ret = safexcel_sha256_init(areq);

	if (ret)
		return ret;

	return safexcel_ahash_finup(areq);
}

static int safexcel_sha256_export(struct ahash_request *areq, void *out)
{
	struct crypto_ahash *ahash = crypto_ahash_reqtfm(areq);
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	struct sha256_state *sha256 = out;
	int len = req->len;
	int cache_len = do_div(len, crypto_ahash_blocksize(ahash));

	sha256->count = req->len;
	memcpy(sha256->state, req->state, req->state_sz);
	memset(sha256->buf, 0, crypto_ahash_blocksize(ahash));
	memcpy(sha256->buf, req->cache, cache_len);

	return 0;
}

static int safexcel_sha256_import(struct ahash_request *areq, const void *in)
{
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);
	const struct sha256_state *sha256 = in;

	memset(req, 0, sizeof(*req));

	req->len = sha256->count;
	memcpy(req->cache, sha256->buf, sha256->count);
	memcpy(req->state, sha256->state, req->state_sz);

	return 0;
}

struct safexcel_alg_template safexcel_alg_sha256 = {
	.type = SAFEXCEL_ALG_TYPE_AHASH,
	.alg.ahash = {
		.init = safexcel_sha256_init,
		.update = safexcel_ahash_update,
		.final = safexcel_ahash_final,
		.finup = safexcel_ahash_finup,
		.digest = safexcel_sha256_digest,
		.export = safexcel_sha256_export,
		.import = safexcel_sha256_import,
		.halg = {
			.digestsize = SHA256_DIGEST_SIZE,
			.statesize = sizeof(struct sha256_state),
			.base = {
				.cra_name = "sha256",
				.cra_driver_name = "safexcel-sha256",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC |
					     CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize = SHA256_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct safexcel_ahash_ctx),
				.cra_init = safexcel_ahash_cra_init,
				.cra_exit = safexcel_ahash_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
};

static int safexcel_sha224_init(struct ahash_request *areq)
{
	struct safexcel_ahash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	struct safexcel_ahash_req *req = ahash_request_ctx(areq);

	memset(req, 0, sizeof(*req));

	req->state[0] = SHA224_H0;
	req->state[1] = SHA224_H1;
	req->state[2] = SHA224_H2;
	req->state[3] = SHA224_H3;
	req->state[4] = SHA224_H4;
	req->state[5] = SHA224_H5;
	req->state[6] = SHA224_H6;
	req->state[7] = SHA224_H7;

	ctx->alg = CONTEXT_CONTROL_CRYPTO_ALG_SHA224;
	ctx->digest = CONTEXT_CONTROL_DIGEST_PRECOMPUTED;
	req->state_sz = SHA256_DIGEST_SIZE;

	return 0;
}

static int safexcel_sha224_digest(struct ahash_request *areq)
{
	int ret = safexcel_sha224_init(areq);

	if (ret)
		return ret;

	return safexcel_ahash_finup(areq);
}

struct safexcel_alg_template safexcel_alg_sha224 = {
	.type = SAFEXCEL_ALG_TYPE_AHASH,
	.alg.ahash = {
		.init = safexcel_sha224_init,
		.update = safexcel_ahash_update,
		.final = safexcel_ahash_final,
		.finup = safexcel_ahash_finup,
		.digest = safexcel_sha224_digest,
		.export = safexcel_sha256_export,
		.import = safexcel_sha256_import,
		.halg = {
			.digestsize = SHA224_DIGEST_SIZE,
			.statesize = sizeof(struct sha256_state),
			.base = {
				.cra_name = "sha224",
				.cra_driver_name = "safexcel-sha224",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC |
					     CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize = SHA224_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct safexcel_ahash_ctx),
				.cra_init = safexcel_ahash_cra_init,
				.cra_exit = safexcel_ahash_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
};
