// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2016, Google, Inc. All rights reserved
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT


#include <err.h>
#include <dev/broadwell_chipset_config.h>
#include <dev/pcie.h>
#include <lk/init.h>
#include <sys/types.h>
#include <trace.h>

extern bool has_cros_embedded_controller(void);

static const struct {
    bwcc_device_id_t dev_id;
    const char*      dev_name;
} DEVICES_TO_UNHIDE[] = {
    { .dev_id = BWCC_DEV_SERIAL_DMA_IO, .dev_name = "Serial DMA" },
    { .dev_id = BWCC_DEV_I2C0,          .dev_name = "I2C0" },
    { .dev_id = BWCC_DEV_SST,           .dev_name = "Smart Sound DSP" },
};

void pixel2_init_quirks(uint level)
{
    /* If we do not have a CrOS embedded controller, then this cannot be a pixel
     * 2, so we should not try to un-hide these devices. */
    if (!has_cros_embedded_controller())
        return;

    status_t res = NO_ERROR;
    for (size_t i = 0; i < countof(DEVICES_TO_UNHIDE); ++i) {
        res = bwcc_disable_device(DEVICES_TO_UNHIDE[i].dev_id, false);
        if (res != NO_ERROR) {
            TRACEF("Failed to enable %s! (res = %d)\n", DEVICES_TO_UNHIDE[i].dev_name, res);
            break;
        }

        res = bwcc_hide_device(DEVICES_TO_UNHIDE[i].dev_id, false);
        if (res != NO_ERROR) {
            TRACEF("Failed to un-hide %s! (res = %d)\n", DEVICES_TO_UNHIDE[i].dev_name, res);
            break;
        }
    }

    /* If something goes terribly wrong, do our best to hide and disable the
     * devices we were trying to unhide.  Otherwise, trigger a rescan of the
     * PCIe bus in order to discover the devices we just un-hid */
    if (res != NO_ERROR) {
        for (size_t i = 0; i < countof(DEVICES_TO_UNHIDE); ++i) {
            bwcc_hide_device(DEVICES_TO_UNHIDE[i].dev_id, true);
            bwcc_disable_device(DEVICES_TO_UNHIDE[i].dev_id, true);
        }
    } else {
        pcie_rescan_bus();
    }
}

LK_INIT_HOOK(pixel2_quirks, &pixel2_init_quirks, LK_INIT_LEVEL_TARGET + 10);

