#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "byte_stream.h"
#include "message.h"
#include "contract.h"
#include "display.h"


void handleSignTransaction(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);

    VALIDATE(p1 == P1_CONFIRM && p2 == 0, ERR_INVALID_REQUEST);
    SignTransactionContext_t* context = &data_context.sign_tr_context;

    size_t offset = 0;

    context->account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(context->account_number);

    context->origin_wallet_type = dataBuffer[offset];
    offset += sizeof(context->origin_wallet_type);

    context->decimals = dataBuffer[offset];
    offset += sizeof(context->decimals);

    uint8_t ticker_len = dataBuffer[offset];
    offset += sizeof(ticker_len);

    VALIDATE(ticker_len != 0 && ticker_len <= MAX_TICKER_LEN, ERR_TICKER_LENGTH);

    memcpy(context->ticker, dataBuffer + offset, ticker_len);
    offset += ticker_len;

    uint8_t metadata = dataBuffer[offset];
    offset += sizeof(metadata);

    // Read wallet type if present
    if (metadata & FLAG_WITH_WALLET_ID) {
        context->current_wallet_type = dataBuffer[offset];
        offset += sizeof(context->current_wallet_type);
    } else {
        context->current_wallet_type = context->origin_wallet_type;
    }

    // Get address
    uint8_t address[ADDRESS_LENGTH];
    get_address(context->account_number, context->origin_wallet_type, address);
    memset(&boc_context, 0, sizeof(boc_context));

    // Read wc if present
    uint8_t wc = DEFAULT_WORKCHAIN_ID;
    if (metadata & FLAG_WITH_WORKCHAIN_ID) {
        wc = dataBuffer[offset];
        offset += sizeof(wc);
    }

    // Read initial address if present
    uint8_t prepend_address[ADDRESS_LENGTH];
    if (metadata & FLAG_WITH_ADDRESS) {
        memcpy(prepend_address, dataBuffer + offset, ADDRESS_LENGTH);
        offset += sizeof(address);
    } else {
        memcpy(prepend_address, address, ADDRESS_LENGTH);
    }

    // Read chain id if present
    if (metadata & FLAG_WITH_CHAIN_ID) {
        context->sign_with_chain_id = true;

        memcpy(context->chain_id, dataBuffer + offset, CHAIN_ID_LENGTH);
        offset += sizeof(context->chain_id);
    } else {
        context->sign_with_chain_id = false;
    }

    uint8_t* msg_begin = dataBuffer + offset;
    uint8_t msg_length = dataLength - offset;

    ByteStream_t src;
    ByteStream_init(&src, msg_begin, msg_length);

    int flow = prepare_to_sign(&src, wc, address, prepend_address);

    ui_display_sign_transaction(flow);

    *flags |= IO_ASYNCH_REPLY;
}
