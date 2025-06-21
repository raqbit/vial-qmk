// Copyright 2024 splitkb.com (support@splitkb.com)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rev1.h"
#include "spi_master.h"
#include "myriad.h"

bool is_oled_enabled = true;

//// HW init

// Make sure all external hardware is
// in a known-good state on powerup
void keyboard_pre_init_kb(void) {
    /// SPI Chip Select pins for various hardware
    // Matrix CS
    gpio_set_pin_output(GP13);
    gpio_write_pin_high(GP13);
    // Myriad Module CS
    gpio_set_pin_output(GP9);
    gpio_write_pin_high(GP9);

    gpio_set_pin_output(ELORA_CC1_PIN);
    gpio_write_pin_low(ELORA_CC1_PIN);

    gpio_set_pin_output(ELORA_CC2_PIN);
    gpio_write_pin_low(ELORA_CC2_PIN);

    // We have to get the SPI interface working quite early,
    // So make sure it is available well before we need it
    spi_init();
    
    keyboard_pre_init_user();
}

//// Encoder functionality

// The encoders are hooked in to the same shift registers as the switch matrix, so we can just piggyback on that.

// Clone of a variant in quantum/matrix_common.c, but matrix-agnostic
bool mat_is_on(matrix_row_t mat[], uint8_t row, uint8_t col) {
    return (mat[row] & ((matrix_row_t)1 << col));
}

uint8_t encoder_read_pads_from(uint8_t index, bool pad_b, matrix_row_t mat[]) {
    // First two matrix rows:
    //
    // Pin  A   B   C   D   E   F   G   H
    // Left: 
    //   { __, __, __, __, __, __, A1, B1 },
    //   { A3, B3, A2, B2, __, __, __, __ }
    // Right:
    //   { A1, B1, __, __, __, __, __, __ },
    //   { __, __, __, __, A2, B2, A3, B3 }
    //
    // See also rev1.h

    bool pad_value = false;

    if (is_keyboard_left()) {
        if (index == 0) {
            pad_value = pad_b ? mat_is_on(mat, 0, 7) : mat_is_on(mat, 0, 6); // B1, A1 
        } else if (index == 1) {
            pad_value = pad_b ? mat_is_on(mat, 1, 3) : mat_is_on(mat, 1, 2); // B2, A2
        } else if (index == 2) {
            pad_value = pad_b ? mat_is_on(mat, 1, 1) : mat_is_on(mat, 1, 0); // B3, A3
        }
    } else {
        if (index == 0) {
            pad_value = pad_b ? mat_is_on(mat, 0, 1) : mat_is_on(mat, 0, 0); // B1, A1
        } else if (index == 1) {
            pad_value = pad_b ? mat_is_on(mat, 1, 5) : mat_is_on(mat, 1, 4); // B2, A2
        } else if (index == 2) {
            pad_value = pad_b ? mat_is_on(mat, 1, 7) : mat_is_on(mat, 1, 6); // B3, A3
        }
    }

    return pad_value ? 1 : 0;
}

void encoder_quadrature_init_pin(uint8_t index, bool pad_b) {
    // At this point the first matrix scan hasn't happened yet,
    // so we can't use raw_matrix to initialize our encoder state
    // as it contains all zeroes - so we have to do our own first scan
    // The pins for myriad are initialized in myriad.c

    matrix_row_t mat[MATRIX_ROWS];

    encoder_read_pads_from(index, pad_b, mat);
}

extern matrix_row_t raw_matrix[MATRIX_ROWS]; // From quantum/matrix_common.c

uint8_t encoder_quadrature_read_pin(uint8_t index, bool pad_b) {
    // The matrix code already keeps the raw matrix up-to-date,
    // so we only have to read the values from it
    if(index <= 2) {
        return encoder_read_pads_from(index, pad_b, raw_matrix);
    } else {
        #ifdef MYRIAD_ENABLE
            return myriad_hook_encoder(index, pad_b);
        #endif
        return 0;
    }
    return 0;
}

//// Default functionality

#ifdef OLED_ENABLE
oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_left()) {
        return OLED_ROTATION_270;
    } else {
        return OLED_ROTATION_90;
    }
}

bool oled_task_kb(void) {
    if (!oled_task_user()) {
        return false;
    }

    if (!is_oled_enabled) {
        oled_off();
        return false;
    } else  {
        oled_on();
    }

    if (!is_keyboard_master()) {
        oled_write_P(PSTR("Elora rev1\n\n"), false);

        // Keyboard Layer Status
        // Ideally we'd print the layer name, but no way to know that for sure
        // Fallback option: just print the layer number
        uint8_t layer = get_highest_layer(layer_state | default_layer_state);
        oled_write_P(PSTR("Layer: "), false);
        oled_write(get_u8_str(layer, ' '), false);

        // Keyboard LED Status
        led_t led_state = host_keyboard_led_state();
        oled_write_P(led_state.num_lock ? PSTR(" NUM") : PSTR("    "), false);
        oled_write_P(led_state.caps_lock ? PSTR("CAP") : PSTR("   "), false);
        oled_write_P(led_state.scroll_lock ? PSTR("SCR") : PSTR("   "), false);

        // QMK Logo
        // clang-format off
       	static const char PROGMEM cactus_logo[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xfc, 0xfe, 0xff, 
			0xff, 0xff, 0xff, 0xfe, 0xfc, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xfe, 0xff, 0xff, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xff, 0xff, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0xff, 0xff, 0xff, 
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 
			0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
       	};
        // clang-format on
        oled_set_cursor(0, oled_max_lines()-8);
        oled_write_raw_P(cactus_logo, sizeof(cactus_logo));
    }

    return false;
}

void housekeeping_task_kb(void) {
    is_oled_enabled = last_input_activity_elapsed() < 60000;
}
#endif

#ifdef ENCODER_ENABLE
bool encoder_update_kb(uint8_t index, bool clockwise) {
    if (!encoder_update_user(index, clockwise)) {
        return false;
    }

    if (index == 0 || index == 1 || index == 2) {
        // Left side
        // Arrow keys
        if (clockwise) {
            tap_code(KC_RIGHT);
        } else {
            tap_code(KC_LEFT);
        }
    } else if (index == 4 || index == 5 || index == 6) {
        // Right side
        // Page up/Page down
        if (clockwise) {
            tap_code(KC_PGDN);
        } else {
            tap_code(KC_PGUP);
        }
    } else if (index == 3 || index == 7) {
        // Myriad
        // Volume control
        if (clockwise) {
            tap_code(KC_VOLU);
        } else {
            tap_code(KC_VOLD);
        }
    }
    return true;
}
#endif
