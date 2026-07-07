#include "quantum.h"
#include "default_keyboard.h"

#include <stdint.h>
#include <stdbool.h>

#define KC_ENC_CW KC_RIGHT
#define KC_ENC_CCW KC_LEFT

#define LAYER_MAIN 0
#define LAYER_GAME 1
#define LAYER_CTRL 2
#define LAYER_NUM 3
#define NUM_LAYERS 4

static const uint16_t CW_KEYCODES[] = {
    [LAYER_MAIN] = KC_RIGHT,
    [LAYER_CTRL] = KC_DOWN,
    [LAYER_GAME] = KC_RIGHT,
    [LAYER_NUM]  = KC_RIGHT,
};

static const uint16_t CCW_KEYCODES[] = {
    [LAYER_MAIN] = KC_LEFT,
    [LAYER_CTRL] = KC_UP,
    [LAYER_GAME] = KC_LEFT,
    [LAYER_NUM]  = KC_LEFT,
};

typedef enum EncEvent {
    ENC_PIN_A_HIGH,
    ENC_PIN_A_LOW,
    ENC_PIN_B_HIGH,
    ENC_PIN_B_LOW,
} EncEvent;

static int handle_enc_event(EncEvent evt) {
    // 'reverse' meaning this array contains graycode indices, indexed by graycode value
    static const uint8_t REVERSE_GRAYCODE[4] = {[0b00] = 0, [0b01] = 1, [0b11] = 2, [0b10] = 3};

#define GET_GRAYCODE_IDX(a, b) REVERSE_GRAYCODE[(((a) & 1) << 1) | ((b) & 1)]

    static uint8_t pin_a = 0;
    static uint8_t pin_b = 0;

    const uint8_t old_pin_a = pin_a;
    const uint8_t old_pin_b = pin_b;

    switch (evt) {
        case ENC_PIN_A_HIGH:
            pin_a = 1;
            break;
        case ENC_PIN_A_LOW:
            pin_a = 0;
            break;
        case ENC_PIN_B_HIGH:
            pin_b = 1;
            break;
        case ENC_PIN_B_LOW:
            pin_b = 0;
            break;
    }

    const uint8_t old_idx = GET_GRAYCODE_IDX(old_pin_a, old_pin_b);
    const uint8_t new_idx = GET_GRAYCODE_IDX(pin_a, pin_b);

    const uint8_t cw_idx  = (old_idx + 1) & 0b11;
    const uint8_t ccw_idx = (old_idx - 1) & 0b11;

    // if the new idx is directly cw or ccw of previous, and the change resulted in rollover, we'll return a delta
    if ((new_idx == cw_idx) && (old_idx == 3)) return 1;
    if ((new_idx == ccw_idx) && (old_idx == 0)) return -1;

    return 0;

#undef GET_GRAYCODE_IDX
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    int delta = 0;

    switch (keycode) {
        case KC_ENC_A:
            delta += handle_enc_event((record->event.pressed ? ENC_PIN_A_HIGH : ENC_PIN_A_LOW));
            break;
        case KC_ENC_B:
            delta += handle_enc_event((record->event.pressed ? ENC_PIN_B_HIGH : ENC_PIN_B_LOW));
            break;
        default:
            return true;
    }

    const uint8_t layer_idx = layer_switch_get_layer(record->event.key);
    if (layer_idx >= NUM_LAYERS) return false; // unknown layer, do nothing

    while (delta > 0) {
        tap_code16(CW_KEYCODES[layer_idx]);
        delta--;
    }
    while (delta < 0) {
        tap_code16(CCW_KEYCODES[layer_idx]);
        delta++;
    }

    return false;
}
