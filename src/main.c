/*******************************************************************************
 *   Ledger Free TON App
 *   (c) 2016 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include "utils.h"
#include "io.h"
#include "apdu_constants.h"
#include "parser.h"
#include "errors.h"
#include "ui/menu.h"
#include "apdu/dispatcher.h"

BocContext_t boc_context;
DataContext_t data_context;

void reset_app_context() {
    memset(&boc_context, 0, sizeof(boc_context));
    memset(&data_context, 0, sizeof(data_context));
}

void app_main(void) {
    volatile unsigned int flags = 0;

    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;

    io_init();

    ui_main_menu();

    // Stores the information about the current command. Some commands expect
    // multiple APDUs before they become complete and executed.
    reset_app_context();
    // reset_spi_buffer();

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        // Receive command bytes in G_io_apdu_buffer
        if ((input_len = io_recv_command()) < 0) {
            PRINTF("=> io_recv_command failure\n");
            return;
        }
        // Parse APDU command from G_io_apdu_buffer
        if (!apdu_parser(&cmd, G_io_apdu_buffer, input_len)) {
            PRINTF("=> /!\\ BAD LENGTH: %.*H\n", input_len, G_io_apdu_buffer);
            io_send_sw(ERR_WRONG_DATA_LENGTH);
            continue;
        }
        PRINTF("=> CLA=%02X | INS=%02X | P1=%02X | P2=%02X | Lc=%02X | CData=%.*H\n",
               cmd.cla,
               cmd.ins,
               cmd.p1,
               cmd.p2,
               cmd.lc,
               cmd.lc,
               cmd.data);
        BEGIN_TRY {
            TRY {
                // Dispatch structured APDU command to handler
                if (apdu_dispatcher(&cmd, &flags) < 0) {
                    PRINTF("=> apdu_dispatcher failure\n");
                    return;
                }
            }
            CATCH_OTHER(e) {
                PRINTF("=> exception: %d\n", e);
                io_send_sw(e);
                return;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}