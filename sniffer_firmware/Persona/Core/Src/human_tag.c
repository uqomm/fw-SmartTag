/*
 * sniffer_tag.c
 *
 *  Created on: May 2, 2024
 *      Author: uqommdev
 */

#include <human_tag.h>

char *TAG_MESSAGES[] = { "NO_RESPONSE", "NO_RXCG_DETECTED", "RX_TIMEOUT",
		"RX_CRC_VALID", "RX_ERROR", "RX_DATA_ZERO", "RX_COMMAND_ERROR",
		"TX_ERROR", "SLEEP", "WAKE_UP", "WAIT_FOR_FIRST_DETECTION",
		"WAIT_FOR_TIMESTAMP_QUERY",	"TAG_TX_SUCCESS",
		"TAG_WRONG_ID_MATCH" };


TAG_STATUS_t process_first_tag_information(TAG_t *tag) {
	TAG_STATUS_t status_reg = 0;
	tag->command = TAG_ID_QUERY;

	start_tag_reception_inmediate(0, 0);
	status_reg = wait_rx_data();
	if (status_reg != TAG_RX_CRC_VALID)
		return (status_reg);

	uint32_t rx_buffer_size = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
	if (rx_buffer_size == 0)
		return (TAG_RX_DATA_ZERO);

	uint8_t rx_buffer[3]; // TODO Revisar RX Buffer primera recepcion
	dwt_readrxdata(rx_buffer, (uint16_t) rx_buffer_size, 0);

	uint8_t received_command = rx_buffer[0];
	tag->sniffer_state = rx_buffer[1];
	tag->sleep_time = rx_buffer[2];

	if (tag->command != received_command)
		return (TAG_RX_COMMAND_ERROR);



	tag->poll_rx_timestamp = get_rx_timestamp_u64();
	/** Set send time for response */
	uint32_t resp_tx_time = (uint32_t) ((tag->poll_rx_timestamp
			+ ((POLL_RX_TO_RESP_TX_DLY_UUS_6M8) * UUS_TO_DWT_TIME))
			>> RESPONSE_TX_TIME_SHIFT_AMOUNT);
	dwt_setdelayedtrxtime(resp_tx_time);

	/** Calculate the response TX timestamp */
	tag->resp_tx_timestamp = (((uint64_t) (resp_tx_time
			& RESPONSE_TX_TIME_MASK_VALUE)) << RESPONSE_TX_TIME_SHIFT_AMOUNT)
			+ TX_ANT_DLY_LP;

	/** Calculate the size needed for the response message buffer */
	uint8_t tx_buffer_size = TX_BUFFER_SIZE;
	uint8_t tx_buffer[TX_BUFFER_SIZE] = { 0 };

	int index = 0;
	tx_buffer[index++] = tag->command;
	*(uint32_t*) (tx_buffer + index) = tag->id;
	index += sizeof(uint32_t);
	*(uint32_t*) (tx_buffer + index) = tag->poll_rx_timestamp;
	index += sizeof(uint32_t);
	*(uint32_t*) (tx_buffer + index) = tag->resp_tx_timestamp;
	index += sizeof(uint32_t);
	*(uint16_t*) (tx_buffer + index) = tag->Voltaje_Bat;


	if (dwt_writetxdata(tx_buffer_size, tx_buffer, 0) == DWT_ERROR) /* Zero offset in TX buffer. */ //MODIFICAR TAMAÑO DE BUFFER
		return (TAG_TX_ERROR);
	dwt_writetxfctrl(tx_buffer_size + 2, 0, 1);
	/*DWT_START_TX_DELAYED DWT_START_TX_IMMEDIATE*/
	if (dwt_starttx(DWT_START_TX_DELAYED) == DWT_ERROR){ //DWT_START_TX_DELAYED o DWT_START_TX_IMMEDIATE
		return (TAG_TX_ERROR);
	}
	//return (TAG_WAIT_FOR_FIRST_DETECTION);
	if (tag->sniffer_state == MASTER_ONE_DETECTION){
		return (TAG_WAIT_SEND_TX);
	}
	return (TAG_WAIT_FOR_TIMESTAMPT_QUERY);
}

TAG_STATUS_t process_second(TAG_t *tag) {
	TAG_STATUS_t status_reg = 0;
	tag->command = TAG_SET_SLEEP_MODE;

	start_tag_reception_inmediate(0, 0);
	status_reg = wait_rx_data();
	if (status_reg != TAG_RX_CRC_VALID)
		return (status_reg);

	uint32_t rx_buffer_size = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;

	if (rx_buffer_size == 0)
		return (TAG_RX_DATA_ZERO);

	if ((rx_buffer_size < 5))
		return (TAG_WAIT_FOR_FIRST_DETECTION);


	uint8_t rx_buffer[5];
	dwt_readrxdata(rx_buffer, (uint16_t) rx_buffer_size, 0);

	uint8_t received_command = rx_buffer[0];
	uint32_t received_id = *(uint32_t*) (rx_buffer + 1);

	if (tag->command != received_command)
		return (TAG_RX_COMMAND_ERROR);

	if (tag->id != received_id)
		return (TAG_WRONG_ID_MATCH);

	return (TAG_SLEEP);
}

TAG_STATUS_t process_queried_tag_information(TAG_t *tag) {
	TAG_STATUS_t status_reg = 0;


	start_tag_reception_inmediate(0, 0);
	status_reg = wait_rx_data();
	if (status_reg != TAG_RX_CRC_VALID)
		return (status_reg);

	uint32_t rx_buffer_size = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
	uint8_t rx_buffer[RX_BUFFER_SIZE_TIMESTAMP_QUERY];
	if (rx_buffer_size == 0)
		return (TAG_RX_DATA_ZERO);
	if ((rx_buffer_size < 5)) //Revisar && (tag->command != TAG_SET_SLEEP_MODE)
		return (TAG_WAIT_FOR_TIMESTAMPT_QUERY);

	dwt_readrxdata(rx_buffer, (uint16_t) rx_buffer_size, 0);

	tag->command = rx_buffer[0];
	uint32_t received_id = *(uint32_t*) (rx_buffer + sizeof(tag->command));
	tag->distance_a = *(uint16_t*) (rx_buffer + sizeof(tag->command) + sizeof(tag->id));
	tag->distance_b = *(uint16_t*) (rx_buffer + sizeof(tag->command) + sizeof(tag->id) + sizeof(tag->distance_a));

	if (tag->id != received_id)
		return (TAG_WRONG_ID_MATCH);
	if (tag->command == TAG_TIMESTAMP_QUERY)
		return (process_response(tag));
	else if(tag->command == TAG_SET_SLEEP_MODE)
		return (process_response(tag));

	return(TAG_RX_COMMAND_ERROR);

}

TAG_STATUS_t process_response(TAG_t *tag) {
	uint8_t tx_buffer_size = TX_BUFFER_SIZE;
	uint8_t tx_buffer[TX_BUFFER_SIZE] = { 0 };
	int index = 0;
	uint64_t poll_rx_timestamp = get_rx_timestamp_u64();

	/** Set send time for response */
	uint32_t resp_tx_time = (uint32_t) ((poll_rx_timestamp
			+ ((POLL_RX_TO_RESP_TX_DLY_UUS_6M8) * UUS_TO_DWT_TIME))
			>> RESPONSE_TX_TIME_SHIFT_AMOUNT);
	dwt_setdelayedtrxtime(resp_tx_time);

	/** Calculate the response TX timestamp */
	uint64_t resp_tx_timestamp = (((uint64_t) (resp_tx_time
			& RESPONSE_TX_TIME_MASK_VALUE)) << RESPONSE_TX_TIME_SHIFT_AMOUNT)
			+ TX_ANT_DLY_LP;
	/** Calculate the size needed for the response message buffer */
	index = 0;
	tx_buffer[index++] = tag->command;
	*(uint32_t*) (tx_buffer + index) = poll_rx_timestamp;
	index += sizeof(uint32_t);
	*(uint32_t*) (tx_buffer + index) = resp_tx_timestamp;
	index += sizeof(uint32_t);
	*(uint16_t*) (tx_buffer + index) = tag->Voltaje_Bat;

	tag->poll_rx_timestamp = poll_rx_timestamp;
	tag->resp_tx_timestamp = resp_tx_timestamp;

	if (dwt_writetxdata(tx_buffer_size, tx_buffer, 0) == DWT_ERROR) /* Zero offset in TX buffer. */
		return (TAG_TX_ERROR);
	dwt_writetxfctrl(tx_buffer_size + 2, 0, 1);
	/*DWT_START_TX_DELAYED DWT_START_TX_IMMEDIATE*/
	if (dwt_starttx(DWT_START_TX_DELAYED) == DWT_ERROR)
		return (TAG_TX_ERROR);

	return (TAG_TX_SUCCESS);
}

uint32_t send_response_with_timestamps(uint8_t *tx_resp_msg, uint8_t size,
		uint32_t frame_seq_nb) {
	uint32_t resp_tx_time;
	uint64_t resp_tx_timestamp;
	uint64_t poll_rx_timestamp;
	int ret;
	uint32_t status_reg = 0;

	/* Retrieve poll reception timestamp. */
	poll_rx_timestamp = get_rx_timestamp_u64();

	/* Set send time for response. See NOTE 9 below. */
	resp_tx_time = (uint32_t) ((poll_rx_timestamp
			+ ((POLL_RX_TO_RESP_TX_DLY_UUS_6M8) * UUS_TO_DWT_TIME)) >> 8);

	/* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
	resp_tx_timestamp = (((uint64_t) (resp_tx_time & 0xFFFFFFFEUL)) << 8)
			+ TX_ANT_DLY_HP;

	/* Write all timestamps in the final message. See NOTE 8 below. */
	resp_msg_set_ts(&tx_resp_msg[FINAL_MSG_POLL_TX_TS_IDX], poll_rx_timestamp);
	resp_msg_set_ts(&tx_resp_msg[FINAL_MSG_RESP_RX_TS_IDX], resp_tx_timestamp);

	tx_resp_msg[ALL_MSG_SN_IDX] = (uint8_t) frame_seq_nb;
	dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
	dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1); /* Zero offset in TX buffer, ranging. */
	/*DWT_START_TX_DELAYED DWT_START_TX_IMMEDIATE*/
	ret = dwt_starttx(
	DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);

	/* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 11 below. */
	if (ret == DWT_SUCCESS) {

		/* Poll for reception of expected "final" frame or error/timeout. See NOTE 8 below. */
		while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID))
				& (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO
						| SYS_STATUS_ALL_RX_ERR))) {

		};
		dwt_setdelayedtrxtime(resp_tx_time);
		/* Set expected delay and timeout for final message reception. See NOTE 4 and 5 below. */
		dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS_6M8);
		/* FINAL_RX_TIMEOUT_UUS. */
		dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS_6M8);
		/* Set preamble timeout for expected frames. See NOTE 6 below. */
		dwt_setpreambledetecttimeout(PRE_TIMEOUT_6M8);

		/* Increment frame sequence number after transmission of the response message (modulo 256). */
		frame_seq_nb++;
	}
	return (status_reg);
}

void start_tag_reception_inmediate(uint8_t preamble_timeout, uint8_t rx_timeout) {

	/* Loop forever responding to ranging requests. */
	dwt_setpreambledetecttimeout(preamble_timeout);
	/* Clear reception timeout to start next ranging process. */
	dwt_setrxtimeout(rx_timeout);
	/* Activate reception immediately. */
	dwt_rxenable(DWT_START_RX_IMMEDIATE);
	/* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
}

#define RX_DATA_TIMEOUT_MS 150 // Timeout in millisecondspasé de 1000 a 100 para probar

TAG_STATUS_t wait_rx_data() {
	uint32_t status_reg;
	uint32_t start_time = HAL_GetTick(); // Get the current time in milliseconds
	uint8_t timeout_reached = 0;

	while (!timeout_reached) {
		if (((status_reg = dwt_read32bitreg(SYS_STATUS_ID))
				& (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO
						| SYS_STATUS_ALL_RX_ERR))) {
			break; // Exit the loop if one of the conditions is met
		}

		// Check for timeout
		if ((HAL_GetTick() - start_time) >= RX_DATA_TIMEOUT_MS) {
			timeout_reached = 1;
		}
	}

	if (timeout_reached) {
		// Handle timeout
		return TAG_RX_TIMEOUT;
	}

	// Check for receive errors
	if ((status_reg & SYS_STATUS_ALL_RX_ERR)) {
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
		return TAG_RX_ERROR;
	}

	// Check for receive timeouts
	if ((status_reg & SYS_STATUS_ALL_RX_TO)) {
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO);
		return TAG_RX_TIMEOUT;
	}

	if ((status_reg & SYS_STATUS_RXFCG_BIT_MASK)) {
		// Clear RX error/timeout events in the DW IC status register
		dwt_write32bitreg(SYS_STATUS_ID,
				SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
		return TAG_RX_CRC_VALID;
	}

	// Clear good RX frame event in the DW IC status register
	dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);


	return TAG_NO_RXCG_DETECTED;
}


void sleep_config(uint16_t sleep_mode, uint16_t mode, uint8_t wake) {
	// Add predefined sleep settings before writing the mode
	sleep_mode |= mode;
	dwt_write16bitoffsetreg(AON_DIG_CFG_ID, 0, pdw3000local->sleep_mode);

	dwt_write8bitoffsetreg(ANA_CFG_ID, 0, wake); //bit 0 - SLEEP_EN, bit 1 - DEEP_SLEEP=0/SLEEP=1, bit 3 wake on CS
}
