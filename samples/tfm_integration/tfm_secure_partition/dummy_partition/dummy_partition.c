/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/crypto.h>
#include <stdbool.h>
#include <stdint.h>
#include "tfm_secure_api.h"
#include "tfm_api.h"

#include "nrf.h"
#include "spu.h"

#include "hal/nrf_gpio.h"

static psa_status_t tfm_dp_init(void)
{
	ERROR_MSG("Accessing GPIOT0\n");
	NRF_GPIOTE0_S->CONFIG[0] =
                (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                (0 << GPIOTE_CONFIG_PSEL_Pos) |
                (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                (0 << GPIOTE_CONFIG_OUTINIT_Pos);
	ERROR_MSG("Accessed GPIOTE0\n");

	ERROR_MSG("Accessing GPIOTE1\n");
	NRF_GPIOTE1_NS->CONFIG[0] =
                (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                (0 << GPIOTE_CONFIG_PSEL_Pos) |
                (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                (0 << GPIOTE_CONFIG_OUTINIT_Pos);
	ERROR_MSG("Accessed GPIOTE1\n");

	SPMLOG_ERRMSGVAL("NRF_P0->PCNF[2] = ", NRF_P0->PIN_CNF[2]);
    	// nrf_gpio_pin_mcu_select(2, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
	SPMLOG_ERRMSGVAL("NRF_P0->PCNF[2] = ", NRF_P0->PIN_CNF[2]);


	// cmse_address_info_t addInfo = cmse_TT((void*)NRF_FICR_S->INFO.CONFIGID);
	// printk("NRF_FICR->CONFIGID", cmse_TT(NRF_FICR_>INFO.CONFIGID).;

	SPMLOG_ERRMSG("address info FICR CONFIGID\n");
	SPMLOG_ERRMSGVAL("mpu_region: ", addInfo.flags.mpu_region);
	SPMLOG_ERRMSGVAL("sau_region: ", addInfo.flags.sau_region);
	SPMLOG_ERRMSGVAL("mpu_region_valid: ", addInfo.flags.mpu_region_valid);
	SPMLOG_ERRMSGVAL("sau_region_valid: ", addInfo.flags.sau_region_valid);
	SPMLOG_ERRMSGVAL("read_ok: ", addInfo.flags.read_ok);
	SPMLOG_ERRMSGVAL("readwrite_ok: ", addInfo.flags.readwrite_ok);
	SPMLOG_ERRMSGVAL("secure: ", addInfo.flags.secure);
	SPMLOG_ERRMSGVAL("idau_region_valid: ", addInfo.flags.idau_region_valid);
	SPMLOG_ERRMSGVAL("idau_region: ", addInfo.flags.idau_region);

	// spu_peripheral_config_secure((uint32_t)NRF_TIMER2, true);

	NRF_TIMER2->TASKS_STOP = 1;
	NRF_TIMER2->TASKS_CLEAR = 1;
	NRF_TIMER2->MODE = (TIMER_MODE_MODE_Counter << TIMER_MODE_MODE_Pos);
	NRF_TIMER2->TASKS_START = 1;



	return TFM_SUCCESS;
}

#define NUM_SECRETS 5

// struct dp_secret {
// 	uint8_t secret[16];
// };

// struct dp_secret secrets[NUM_SECRETS] = {
// 	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
// 	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
// 	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
// 	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
// 	{{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}},
// };

typedef void (*psa_write_callback_t)(void *handle, uint8_t *digest,
				uint32_t digest_size);

static psa_status_t tfm_dp_secret_digest(uint32_t secret_index,
			size_t digest_size, size_t *p_digest_size,
			psa_write_callback_t callback, void *handle)
{
	uint8_t digest[32], secret[16];
	psa_status_t status;

	/* Check that secret_index is valid. */
	if (secret_index >= NUM_SECRETS) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Check that digest_size is valid. */
	if (digest_size != sizeof(digest)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	secret[0] = (uint8_t)secret_index;

	NRF_TIMER2->TASKS_CLEAR = 1;
	for (int i = 1; i < sizeof(secret); i++)
	{
		NRF_TIMER2->TASKS_COUNT = 1;
		NRF_TIMER2->TASKS_CAPTURE[0] = 1;
		secret[i] = NRF_TIMER2->CC[0];

		// secret[i] = i;
	}

	status = psa_hash_compute(PSA_ALG_SHA_256, secret,
				sizeof(secret), digest,
				digest_size, p_digest_size);

	if (status != PSA_SUCCESS) {
		return status;
	}
	if (*p_digest_size != digest_size){
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	callback(handle, digest, digest_size);

	return PSA_SUCCESS;
}

#ifndef TFM_PSA_API

#include "tfm_memory_utils.h"

/*
 * \brief Indicates whether DP has been initialised.
 */
static bool dp_is_init = false;

/*
 * \brief Initialises DP, if not already initialised.
 *
 * \note In library mode, initialisation is delayed until the first secure
 *	   function call, as calls to the Crypto service are required for
 *	   initialisation.
 *
 * \return PSA_SUCCESS if DP is initialised, PSA_ERROR_GENERIC_ERROR
 *		 otherwise.
 */
static psa_status_t dp_check_init(void)
{
	if (!dp_is_init) {
		if (tfm_dp_init() != PSA_SUCCESS) {
			return PSA_ERROR_GENERIC_ERROR;
		}
		dp_is_init = true;
	}

	return PSA_SUCCESS;
}


void psa_write_digest(void *handle, uint8_t *digest, uint32_t digest_size)
{
	tfm_memcpy(handle, digest, digest_size);
}

psa_status_t tfm_dp_secret_digest_req(psa_invec *in_vec, size_t in_len,
				psa_outvec *out_vec, size_t out_len)
{
	uint32_t secret_index;

	if (dp_check_init() != PSA_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	if ((in_len != 1) || (out_len != 1)) {
		/* The number of arguments are incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (in_vec[0].len != sizeof(secret_index)) {
		/* The input argument size is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	secret_index = *((uint32_t *)in_vec[0].base);

	return tfm_dp_secret_digest(
				secret_index, out_vec[0].len, &out_vec[0].len,
				psa_write_digest, (void *)out_vec[0].base);
}

#else /* !defined(TFM_PSA_API) */
#include "psa/service.h"
#include "psa_manifest/tfm_dummy_partition.h"

typedef psa_status_t (*dp_func_t)(psa_msg_t *);

static void psa_write_digest_0(void *handle, uint8_t *digest,
				uint32_t digest_size)
{
	psa_write((psa_handle_t)handle, 0, digest, digest_size);
}

static psa_status_t tfm_dp_secret_digest_ipc(psa_msg_t *msg)
{
	size_t num = 0;
	uint32_t secret_index;

	if (msg->in_size[0] != sizeof(secret_index)) {
		/* The size of the argument is incorrect */
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	num = psa_read(msg->handle, 0, &secret_index, msg->in_size[0]);
	if (num != msg->in_size[0]) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	return tfm_dp_secret_digest(
				secret_index, msg->out_size[0], &msg->out_size[0],
				psa_write_digest_0, (void *)msg->handle);
}


static void dp_signal_handle(psa_signal_t signal, dp_func_t pfn)
{
	psa_status_t status;
	psa_msg_t msg;

	status = psa_get(signal, &msg);
	switch (msg.type) {
	case PSA_IPC_CONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	case PSA_IPC_CALL:
		status = pfn(&msg);
		psa_reply(msg.handle, status);
		break;
	case PSA_IPC_DISCONNECT:
		psa_reply(msg.handle, PSA_SUCCESS);
		break;
	default:
		psa_panic();
	}
}
#endif /* !defined(TFM_PSA_API) */

psa_status_t tfm_dp_req_mngr_init(void)
{
#ifdef TFM_PSA_API
	psa_signal_t signals = 0;

	if (tfm_dp_init() != PSA_SUCCESS) {
		psa_panic();
	}

	while (1) {
		signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
		if (signals & TFM_DP_SECRET_DIGEST_SIGNAL) {
			dp_signal_handle(TFM_DP_SECRET_DIGEST_SIGNAL,
					tfm_dp_secret_digest_ipc);
		} else {
			psa_panic();
		}
	}
#else
	/* In library mode, initialisation is delayed until the first secure
	 * function call, as calls to the Crypto service are required for
	 * initialisation.
	 */
	return PSA_SUCCESS;
#endif
}