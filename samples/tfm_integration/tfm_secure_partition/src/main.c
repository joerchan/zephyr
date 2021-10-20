/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tfm_veneers.h>
#include <tfm_ns_interface.h>

#include <arm_cmse.h>
// #include "nrf.h"
#ifndef CONFIG_TFM_IPC
psa_status_t dp_secret_digest(uint32_t secret_index,
			void *p_digest,
			size_t digest_size)
{
	psa_status_t status;
	psa_invec in_vec[] = {
		{ .base = &secret_index, .len = sizeof(secret_index) },
	};

	psa_outvec out_vec[] = {
		{ .base = p_digest, .len = digest_size }
	};

	status = tfm_ns_interface_dispatch(
				(veneer_fn)tfm_dp_secret_digest_req_veneer,
				(uint32_t)in_vec,  IOVEC_LEN(in_vec),
				(uint32_t)out_vec, IOVEC_LEN(out_vec));

	return status;
}

#else
#include "psa/client.h"
#include "psa_manifest/sid.h"

psa_status_t dp_secret_digest(uint32_t secret_index,
			void *p_digest,
			size_t digest_size)
{
	psa_status_t status;
	psa_handle_t handle;

	psa_invec in_vec[] = {
		{ .base = &secret_index, .len = sizeof(secret_index) },
	};

	psa_outvec out_vec[] = {
		{ .base = p_digest, .len = digest_size }
	};

	handle = psa_connect(TFM_DP_SECRET_DIGEST_SID,
				TFM_DP_SECRET_DIGEST_VERSION);
	if (!PSA_HANDLE_IS_VALID(handle)) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	status = psa_call(handle, PSA_IPC_CALL, in_vec, IOVEC_LEN(in_vec),
			out_vec, IOVEC_LEN(out_vec));

	psa_close(handle);

	return status;
}


#endif
#include "nrf.h"
#include "hal/nrf_gpio.h"
void main(void)
{
	uint8_t digest[32];

	for (int key = 0; key < 6; key++) {
		psa_status_t status = dp_secret_digest(key, digest, sizeof(digest));

		if (status != PSA_SUCCESS) {
			printk("Status: %d\n", status);
		} else {
			printk("Digest: ");
			for (int i = 0; i < 32; i++) {
				printk("%02x", digest[i]);
			}
			printk("\n");
		}
	}

	// printk("Accessing GPIOT0\n");
	// NRF_GPIOTE0_S->CONFIG[0] =
        //         (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
        //         (0 << GPIOTE_CONFIG_PSEL_Pos) |
        //         (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
        //         (0 << GPIOTE_CONFIG_OUTINIT_Pos);
	// printk("Accessed GPIOTE0\n");

	printk("Accessing GPIOTE1\n");
	NRF_GPIOTE1->CONFIG[0] =
                (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                (0 << GPIOTE_CONFIG_PSEL_Pos) |
                (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                (0 << GPIOTE_CONFIG_OUTINIT_Pos);
	printk("Accessed GPIOTE1\n");

	// printk("Attempting to access secure peripheral\n");

	// NRF_TIMER2->TASKS_COUNT = 1;
	// NRF_TIMER2->TASKS_CAPTURE[0] = 1;
	// printk("Timer CC: %d\n", NRF_TIMER2->CC[0]);

	// volatile uint32_t ready = NRF_NVMC->READY;
	// printk("Accessed NVMC ready %u\n", ready);
	// volatile uint32_t config = NRF_NVMC->CONFIG;
	// printk("Accessed NVMC config %u\n", config);

	NRF_NVMC_NS->CONFIGNS = 1;
	printk("NS: Enable Write\n");

	NRF_NVMC_NS->CONFIGNS = 0;
	printk("NS: Disable Write\n");


	NRF_NVMC_NS->CONFIG = 1;
	printk("S: Enable Write\n");

	NRF_NVMC_NS->CONFIG = 0;
	printk("S: Disable Write\n");

#define PIN_XL1 0
#define PIN_XL2 1

	printk("NRF_P0->PCNF[0] %d\n", NRF_P0->PIN_CNF[0]);
	printk("NRF_P0->PCNF[1] %d\n", NRF_P0->PIN_CNF[1]);

    	nrf_gpio_pin_mcu_select(PIN_XL1, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
    	nrf_gpio_pin_mcu_select(PIN_XL2, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);

	printk("NRF_P0->PCNF[0] %d\n", NRF_P0->PIN_CNF[0]);
	printk("NRF_P0->PCNF[1] %d\n", NRF_P0->PIN_CNF[1]);

	printk("NRF_P0->PCNF[2] %d\n", NRF_P0->PIN_CNF[2]);
    	NRF_P0->PIN_CNF[2] = 0x01;
	printk("NRF_P0->PCNF[2] %d\n", NRF_P0->PIN_CNF[2]);


	printk("NRF_P1->PCNF[2] %d\n", NRF_P1->PIN_CNF[2]);
    	NRF_P1->PIN_CNF[2] = 0x01;
	printk("NRF_P1->PCNF[2] %d\n", NRF_P1->PIN_CNF[2]);

	printk("GPIO->PIN_CNF accessed\n");
}
