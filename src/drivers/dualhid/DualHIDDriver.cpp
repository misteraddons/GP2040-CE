/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "drivers/dualhid/DualHIDDriver.h"
#include "drivers/dualhid/DualHIDDescriptors.h"
#include "drivers/shared/driverhelper.h"
#include "tusb.h"

static bool dual_hid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
    return hidd_control_xfer_cb(rhport, stage, request);
}

void DualHIDDriver::initialize() {
    hidReportP1 = {
        .buttons = 0,
        .direction = HID_HAT_NOTHING,
        .l_x_axis = HID_JOYSTICK_MID, .l_y_axis = HID_JOYSTICK_MID,
        .r_x_axis = HID_JOYSTICK_MID, .r_y_axis = HID_JOYSTICK_MID,
    };
    
    hidReportP2 = {
        .buttons = 0,
        .direction = HID_HAT_NOTHING,
        .l_x_axis = HID_JOYSTICK_MID, .l_y_axis = HID_JOYSTICK_MID,
        .r_x_axis = HID_JOYSTICK_MID, .r_y_axis = HID_JOYSTICK_MID,
    };

    class_driver = {
    #if CFG_TUSB_DEBUG >= 2
        .name = "DualHID",
    #endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = dual_hid_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL
    };
    
    player2Enabled = false;
}

// Process Player 1 gamepad and send to TUSB Device
void DualHIDDriver::process(Gamepad * gamepad, uint8_t * outBuffer) {
    // Process Player 1 D-pad
    switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD)
    {
        case GAMEPAD_MASK_UP:                        hidReportP1.direction = HID_HAT_UP;        break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:   hidReportP1.direction = HID_HAT_UPRIGHT;   break;
        case GAMEPAD_MASK_RIGHT:                     hidReportP1.direction = HID_HAT_RIGHT;     break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT: hidReportP1.direction = HID_HAT_DOWNRIGHT; break;
        case GAMEPAD_MASK_DOWN:                      hidReportP1.direction = HID_HAT_DOWN;      break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:  hidReportP1.direction = HID_HAT_DOWNLEFT;  break;
        case GAMEPAD_MASK_LEFT:                      hidReportP1.direction = HID_HAT_LEFT;      break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:    hidReportP1.direction = HID_HAT_UPLEFT;    break;
        default:                                     hidReportP1.direction = HID_HAT_NOTHING;   break;
    }

    hidReportP1.l_x_axis = static_cast<uint8_t>(gamepad->state.lx >> 8);
    hidReportP1.l_y_axis = static_cast<uint8_t>(gamepad->state.ly >> 8);
    hidReportP1.r_x_axis = static_cast<uint8_t>(gamepad->state.rx >> 8);
    hidReportP1.r_y_axis = static_cast<uint8_t>(gamepad->state.ry >> 8);

    // Map buttons for Player 1
    hidReportP1.buttons = 0
        | (gamepad->pressedB1()    ? GAMEPAD_MASK_B2     : 0)
        | (gamepad->pressedB2()    ? GAMEPAD_MASK_B3     : 0)
        | (gamepad->pressedB3()    ? GAMEPAD_MASK_B1     : 0)
        | (gamepad->pressedB4()    ? GAMEPAD_MASK_B4     : 0)
        | (gamepad->pressedL1()    ? GAMEPAD_MASK_L1     : 0)
        | (gamepad->pressedR1()    ? GAMEPAD_MASK_R1     : 0)
        | (gamepad->pressedL2()    ? GAMEPAD_MASK_L2     : 0)
        | (gamepad->pressedR2()    ? GAMEPAD_MASK_R2     : 0)
        | (gamepad->pressedS1()    ? GAMEPAD_MASK_S1     : 0)
        | (gamepad->pressedS2()    ? GAMEPAD_MASK_S2     : 0)
        | (gamepad->pressedL3()    ? GAMEPAD_MASK_L3     : 0)
        | (gamepad->pressedR3()    ? GAMEPAD_MASK_R3     : 0)
        | (gamepad->pressedA1()    ? GAMEPAD_MASK_A1     : 0)
        | (gamepad->pressedA2()    ? GAMEPAD_MASK_A2     : 0)
        | (gamepad->pressedA3()    ? GAMEPAD_MASK_A3     : 0)
        | (gamepad->pressedA4()    ? GAMEPAD_MASK_A4     : 0)
        | (gamepad->pressedUp()    ? GAMEPAD_MASK_DU     : 0)
        | (gamepad->pressedDown()  ? GAMEPAD_MASK_DD     : 0)
        | (gamepad->pressedLeft()  ? GAMEPAD_MASK_DL     : 0)
        | (gamepad->pressedRight() ? GAMEPAD_MASK_DR     : 0)
        | (gamepad->pressedE1()    ? GAMEPAD_MASK_E1     : 0)
        | (gamepad->pressedE2()    ? GAMEPAD_MASK_E2     : 0)
        | (gamepad->pressedE3()    ? GAMEPAD_MASK_E3     : 0)
        | (gamepad->pressedE4()    ? GAMEPAD_MASK_E4     : 0)
        | (gamepad->pressedE5()    ? GAMEPAD_MASK_E5     : 0)
        | (gamepad->pressedE6()    ? GAMEPAD_MASK_E6     : 0)
        | (gamepad->pressedE7()    ? GAMEPAD_MASK_E7     : 0)
        | (gamepad->pressedE8()    ? GAMEPAD_MASK_E8     : 0)
        | (gamepad->pressedE9()    ? GAMEPAD_MASK_E9     : 0)
        | (gamepad->pressedE10()   ? GAMEPAD_MASK_E10    : 0)
        | (gamepad->pressedE11()   ? GAMEPAD_MASK_E11    : 0)
        | (gamepad->pressedE12()   ? GAMEPAD_MASK_E12    : 0)
    ;

    // Wake up TinyUSB device
    if (tud_suspended())
        tud_remote_wakeup();

    // Send Player 1 report on interface 0
    void * report = &hidReportP1;
    uint16_t report_size = sizeof(hidReportP1);
    if (memcmp(last_report_p1, report, report_size) != 0)
    {
        // HID ready + report sent, copy previous report
        if (tud_hid_n_ready(PLAYER1_INTERFACE) && tud_hid_n_report(PLAYER1_INTERFACE, 0, report, report_size) == true ) {
            memcpy(last_report_p1, report, report_size);
        }
    }
}

// Process Player 2 gamepad
void DualHIDDriver::processPlayer2(Gamepad * gamepad2) {
    if (!gamepad2 || !player2Enabled) return;
    
    // Process Player 2 D-pad
    switch (gamepad2->state.dpad & GAMEPAD_MASK_DPAD)
    {
        case GAMEPAD_MASK_UP:                        hidReportP2.direction = HID_HAT_UP;        break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:   hidReportP2.direction = HID_HAT_UPRIGHT;   break;
        case GAMEPAD_MASK_RIGHT:                     hidReportP2.direction = HID_HAT_RIGHT;     break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT: hidReportP2.direction = HID_HAT_DOWNRIGHT; break;
        case GAMEPAD_MASK_DOWN:                      hidReportP2.direction = HID_HAT_DOWN;      break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:  hidReportP2.direction = HID_HAT_DOWNLEFT;  break;
        case GAMEPAD_MASK_LEFT:                      hidReportP2.direction = HID_HAT_LEFT;      break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:    hidReportP2.direction = HID_HAT_UPLEFT;    break;
        default:                                     hidReportP2.direction = HID_HAT_NOTHING;   break;
    }

    hidReportP2.l_x_axis = static_cast<uint8_t>(gamepad2->state.lx >> 8);
    hidReportP2.l_y_axis = static_cast<uint8_t>(gamepad2->state.ly >> 8);
    hidReportP2.r_x_axis = static_cast<uint8_t>(gamepad2->state.rx >> 8);
    hidReportP2.r_y_axis = static_cast<uint8_t>(gamepad2->state.ry >> 8);

    // Map buttons for Player 2
    hidReportP2.buttons = 0
        | (gamepad2->pressedB1()    ? GAMEPAD_MASK_B2     : 0)
        | (gamepad2->pressedB2()    ? GAMEPAD_MASK_B3     : 0)
        | (gamepad2->pressedB3()    ? GAMEPAD_MASK_B1     : 0)
        | (gamepad2->pressedB4()    ? GAMEPAD_MASK_B4     : 0)
        | (gamepad2->pressedL1()    ? GAMEPAD_MASK_L1     : 0)
        | (gamepad2->pressedR1()    ? GAMEPAD_MASK_R1     : 0)
        | (gamepad2->pressedL2()    ? GAMEPAD_MASK_L2     : 0)
        | (gamepad2->pressedR2()    ? GAMEPAD_MASK_R2     : 0)
        | (gamepad2->pressedS1()    ? GAMEPAD_MASK_S1     : 0)
        | (gamepad2->pressedS2()    ? GAMEPAD_MASK_S2     : 0)
        | (gamepad2->pressedL3()    ? GAMEPAD_MASK_L3     : 0)
        | (gamepad2->pressedR3()    ? GAMEPAD_MASK_R3     : 0)
        | (gamepad2->pressedA1()    ? GAMEPAD_MASK_A1     : 0)
        | (gamepad2->pressedA2()    ? GAMEPAD_MASK_A2     : 0)
        | (gamepad2->pressedA3()    ? GAMEPAD_MASK_A3     : 0)
        | (gamepad2->pressedA4()    ? GAMEPAD_MASK_A4     : 0)
        | (gamepad2->pressedUp()    ? GAMEPAD_MASK_DU     : 0)
        | (gamepad2->pressedDown()  ? GAMEPAD_MASK_DD     : 0)
        | (gamepad2->pressedLeft()  ? GAMEPAD_MASK_DL     : 0)
        | (gamepad2->pressedRight() ? GAMEPAD_MASK_DR     : 0)
        | (gamepad2->pressedE1()    ? GAMEPAD_MASK_E1     : 0)
        | (gamepad2->pressedE2()    ? GAMEPAD_MASK_E2     : 0)
        | (gamepad2->pressedE3()    ? GAMEPAD_MASK_E3     : 0)
        | (gamepad2->pressedE4()    ? GAMEPAD_MASK_E4     : 0)
        | (gamepad2->pressedE5()    ? GAMEPAD_MASK_E5     : 0)
        | (gamepad2->pressedE6()    ? GAMEPAD_MASK_E6     : 0)
        | (gamepad2->pressedE7()    ? GAMEPAD_MASK_E7     : 0)
        | (gamepad2->pressedE8()    ? GAMEPAD_MASK_E8     : 0)
        | (gamepad2->pressedE9()    ? GAMEPAD_MASK_E9     : 0)
        | (gamepad2->pressedE10()   ? GAMEPAD_MASK_E10    : 0)
        | (gamepad2->pressedE11()   ? GAMEPAD_MASK_E11    : 0)
        | (gamepad2->pressedE12()   ? GAMEPAD_MASK_E12    : 0)
    ;

    // Send Player 2 report on interface 1
    void * report = &hidReportP2;
    uint16_t report_size = sizeof(hidReportP2);
    if (memcmp(last_report_p2, report, report_size) != 0)
    {
        // HID ready + report sent, copy previous report
        if (tud_hid_n_ready(PLAYER2_INTERFACE) && tud_hid_n_report(PLAYER2_INTERFACE, 0, report, report_size) == true ) {
            memcpy(last_report_p2, report, report_size);
        }
    }
    
    player2Enabled = true;
}

// tud_hid_get_report_cb - handle both interfaces
uint16_t DualHIDDriver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    // Determine which interface is requesting
    uint8_t itf = tud_hid_n_interface_protocol_get(0) ? 0 : 1;
    
    if (itf == PLAYER1_INTERFACE) {
        memcpy(buffer, &hidReportP1, sizeof(HIDReport));
    } else if (itf == PLAYER2_INTERFACE) {
        memcpy(buffer, &hidReportP2, sizeof(HIDReport));
    }
    
    return sizeof(HIDReport);
}

// Only PS4 does anything with set report
void DualHIDDriver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

// Only XboxOG and Xbox One use vendor control xfer cb
bool DualHIDDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}

const uint16_t * DualHIDDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    const char *value = (const char *)dual_hid_string_descriptors[index];
    return getStringDescriptor(value, index); // getStringDescriptor returns a static array
}

const uint8_t * DualHIDDriver::get_descriptor_device_cb() {
    return dual_hid_device_descriptor;
}

const uint8_t * DualHIDDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    // Both interfaces use the same report descriptor
    return dual_hid_report_descriptor;
}

const uint8_t * DualHIDDriver::get_descriptor_configuration_cb(uint8_t index) {
    return dual_hid_configuration_descriptor;
}

const uint8_t * DualHIDDriver::get_descriptor_device_qualifier_cb() {
    return nullptr;
}

uint16_t DualHIDDriver::GetJoystickMidValue() {
    return HID_JOYSTICK_MID << 8;
}