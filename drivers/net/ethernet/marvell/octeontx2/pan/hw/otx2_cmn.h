/* otx2_common.c */

int dup_rq_init(struct otx2_nic *pfvf, u16 qidx, u16 lpb_aura);

int dup_alloc_buffer(struct otx2_nic *pfvf, struct otx2_cq_queue *cq,
		     dma_addr_t *dma);

int dup_txschq_config(struct otx2_nic *pfvf, int lvl, int prio,
		      bool txschq_for_pfc);
int dup_smq_flush(struct otx2_nic *pfvf, int smq);

int dup_txsch_alloc(struct otx2_nic *pfvf);
void dup_txschq_stop(struct otx2_nic *pfvf);
void dup_sqb_flush(struct otx2_nic *pfvf);
int dup_sq_aq_init(void *dev, u16 qidx, u8 chan_offset, u16 sqb_aura);
int dup_config_nix_queues(struct otx2_nic *pfvf);
int dup_config_nix(struct otx2_nic *pfvf);
void dup_sq_free_sqbs(struct otx2_nic *pfvf);

void dup_free_bufs(struct otx2_nic *pfvf, struct otx2_pool *pool,
		   u64 iova, int size);
void dup_aura_pool_free(struct otx2_nic *pfvf);
int dup_sq_aura_pool_init(struct otx2_nic *pfvf);
int dup_rq_aura_pool_init(struct otx2_nic *pfvf);
int dup_config_npa(struct otx2_nic *pfvf);
int dup_detach_resources(struct mbox *mbox);
int dup_attach_npa_nix(struct otx2_nic *pfvf);
void dup_ctx_disable(struct mbox *mbox, int type, bool npa);
int dup_nix_config_bp(struct otx2_nic *pfvf, bool enable);
void dup_mbox_handler_cgx_stats(struct otx2_nic *pfvf,
				struct cgx_stats_rsp *rsp);
void dup_mbox_handler_cgx_fec_stats(struct otx2_nic *pfvf,
				    struct cgx_fec_stats_rsp *rsp);
void dup_mbox_handler_npa_lf_alloc(struct otx2_nic *pfvf,
				   struct npa_lf_alloc_rsp *rsp);
void dup_mbox_handler_nix_lf_alloc(struct otx2_nic *pfvf,
				   struct nix_lf_alloc_rsp *rsp);
void dup_mbox_handler_nix_bp_enable(struct otx2_nic *pfvf,
				    struct nix_bp_cfg_rsp *rsp);
void dup_set_cints_affinity(struct otx2_nic *pfvf);

/* otx2_pf.c */

int dup_init_hw_resources(struct otx2_nic *pf);
int dup_get_rbuf_size(struct otx2_nic *pf, int mtu);
void dup_free_queue_mem(struct otx2_qset *qset);

int dup_check_pf_usable(struct otx2_nic *nic);

int dup_pfaf_mbox_init(struct otx2_nic *pf);
int dup_register_mbox_intr(struct otx2_nic *pf, bool probe_af);
int dup_realloc_msix_vectors(struct otx2_nic *pf);
void dup_free_hw_resources(struct otx2_nic *pf);
void dup_disable_mbox_intr(struct otx2_nic *pf);
void dup_pfaf_mbox_destroy(struct otx2_nic *pf);
void dup_otx2_free_aura_ptr(struct otx2_nic *pfvf, int type);

/* otx2_txrx.c */

struct nix_cqe_rx_s;
struct nix_rx_parse_s;

bool dup_check_rcv_errors(struct otx2_nic *pfvf,
			  struct nix_cqe_rx_s *cqe, int qidx);

bool otx2_skb_add_frag(struct otx2_nic *pfvf, struct sk_buff *skb,
		       u64 iova, int len, struct nix_rx_parse_s *parse,
		       int qidx);

void dup_set_rxhash(struct otx2_nic *pfvf,
		    struct nix_cqe_rx_s *cqe, struct sk_buff *skb);

void dup_set_taginfo(struct nix_rx_parse_s *parse,
		     struct sk_buff *skb);

int dup_nix_cq_op_status(struct otx2_nic *pfvf,
			 struct otx2_cq_queue *cq);

struct nix_cqe_hdr_s *dup_get_next_cqe(struct otx2_cq_queue *cq);

void dup_cleanup_rx_cqes(struct otx2_nic *pfvf, struct otx2_cq_queue *cq, int qidx);

void dup_free_pending_sqe(struct otx2_nic *pfvf);

void dup_sqe_flush(void *dev, struct otx2_snd_queue *sq,
		   int size, int qidx);

int dup_refill_pool_ptrs(void *dev, struct otx2_cq_queue *cq);
int dup_cn10k_refill_pool_ptrs(void *dev, struct otx2_cq_queue *cq);

void dup_cn10k_sqe_flush(void *dev, struct otx2_snd_queue *sq,
			 int size, int qidx);
int dup_rxtx_enable(struct otx2_nic *pfvf, bool enable);

/* qos.c */
void dup_clean_qos_queues(struct otx2_nic *pfvf);

/* cn10k.c */
int dup_cn10k_lmtst_init(struct otx2_nic *pfvf);
int dup_cn10k_free_all_ipolicers(struct otx2_nic *pfvf);
int dup_cn10k_alloc_leaf_profile(struct otx2_nic *pfvf, u16 *leaf);
//int dup_cn10k_alloc_matchall_ipolicer(struct otx2_nic *pfvf);

/* newly added */

typedef int (*init_qfn_t)(struct otx2_nic *pfvf, u16 qidx, u16 __maybe_unused aura);
typedef void (*cleanup_qfn_t)(struct otx2_nic *pfvf, struct otx2_cq_queue *cq,
			      int __maybe_unused qidx);
typedef int (*tx_schq_init_t)(struct otx2_nic *pf);
typedef void (*tx_schq_free_one_t)(struct otx2_nic *pfvf, u16 lvl, u16 schq);
struct otx2_cmn_fops {
	init_qfn_t sq_init;
	init_qfn_t rq_init;
	init_qfn_t cq_init;
	cleanup_qfn_t rx_cq_clean;
	cleanup_qfn_t tx_cq_clean;
	tx_schq_init_t tx_schq_init;
	tx_schq_free_one_t tx_schq_free_one;
};

struct otx2_cmn_fops *otx2_cmn_fops_arr_lookup(int pci_dev_id);
void otx2_cmn_fops_arr_add(int pci_dev_id, struct otx2_cmn_fops *ops);
void otx2_cmn_fops_arr_del(int pci_dev_id);
