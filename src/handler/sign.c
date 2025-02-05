#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "display.h"

void handleSign(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);

    VALIDATE(p1 == P1_CONFIRM && p2 == 0, ERR_INVALID_REQUEST);
    SignContext_t* context = &data_context.sign_context;

    size_t offset = 0;

    context->account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(context->account_number);

    uint8_t metadata = dataBuffer[offset];
    offset += sizeof(metadata);

    // Read chain id if present
    if (metadata & FLAG_WITH_CHAIN_ID) {
        context->sign_with_chain_id = true;

        memcpy(context->chain_id, dataBuffer + offset, CHAIN_ID_LENGTH);
        offset += sizeof(context->chain_id);
    }

    if (!context->sign_with_chain_id) {
        memcpy(context->to_sign, dataBuffer + offset, TO_SIGN_LENGTH);
        snprintf(context->to_sign_str, sizeof(context->to_sign_str), "%.*H", TO_SIGN_LENGTH, context->to_sign);
    } else {
        memcpy(context->to_sign, context->chain_id, CHAIN_ID_LENGTH);
        memcpy(context->to_sign + CHAIN_ID_LENGTH, dataBuffer + offset, TO_SIGN_LENGTH);
        snprintf(context->to_sign_str, sizeof(context->to_sign_str), "%.*H", CHAIN_ID_LENGTH + TO_SIGN_LENGTH, context->to_sign);
    }

    ui_display_sign();
    *flags |= IO_ASYNCH_REPLY;
}
