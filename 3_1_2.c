/*************************************************************************
 * MEAM 5100 Lab 3.1.2 - General-purpose ADC subroutines for the ATmega32U4
 * Linqi Zhe
 *
 *   adc_setup(ch)          configure the ADC and select channel ch (0..13)
 *   adc_read()             one 10-bit conversion on the selected channel
 *   adc_read_channel(ch)   optional: switch channel only when it changes,
 *                          discard the first conversion after the switch,
 *                          then read (efficient multiplexing of many pots)
 *
 * Only the set()/clear() macros from MEAM_general.h touch the registers.
 * The test main() at the bottom prints ADC7 (ItsyBitsy A0) over USB and
 * is deleted when the routines are pasted into another program.
 *************************************************************************/
#include "MEAM_general.h"
#include "m_usb.h"

static uint8_t g_adc_channel = 0;      /* channel currently selected in the MUX */

/* Turn the digital input buffer of an analog pin off (less noise, no
 * shoot-through current in the input stage).  ADC0..7 -> DIDR0 bits 0..7,
 * ADC8..13 -> DIDR2 bits 0..5.                                             */
static void adc_disable_digital_input(uint8_t ch)
{
    if (ch <= 7)       { set(DIDR0, ch); }
    else if (ch <= 13) { set(DIDR2, (ch - 8)); }
}

/* adc_setup(ch): AVcc reference, right-adjusted 10-bit result, clk/128
 * (125 kHz at 16 MHz), channel ch selected, converter enabled, polling mode. */
void adc_setup(uint8_t ch)
{
    if (ch > 13) ch = 0;               /* ADC14/15 do not exist on the 32U4 */
    g_adc_channel = ch;

    clear(ADMUX, REFS1);               /* REFS1:0 = 01 -> AVcc (5 V) reference */
    set(ADMUX, REFS0);
    clear(ADMUX, ADLAR);               /* right-adjust: full 10 bits in ADC  */

    clear(ADMUX, MUX0);                /* clear the old channel first       */
    clear(ADMUX, MUX1);
    clear(ADMUX, MUX2);
    clear(ADMUX, MUX3);
    clear(ADCSRB, MUX5);

    if (ch <= 7) {                     /* ADC0..7: MUX5 = 0, MUX2:0 = ch    */
        if (ch & 0x01) set(ADMUX, MUX0);
        if (ch & 0x02) set(ADMUX, MUX1);
        if (ch & 0x04) set(ADMUX, MUX2);
    } else {                           /* ADC8..13: MUX5 = 1, MUX2:0 = ch-8 */
        uint8_t low = (uint8_t)(ch - 8);
        set(ADCSRB, MUX5);
        if (low & 0x01) set(ADMUX, MUX0);
        if (low & 0x02) set(ADMUX, MUX1);
        if (low & 0x04) set(ADMUX, MUX2);
    }
    adc_disable_digital_input(ch);

    set(ADCSRA, ADPS2);                /* ADPS2:0 = 111 -> prescaler 128     */
    set(ADCSRA, ADPS1);
    set(ADCSRA, ADPS0);
    clear(ADCSRA, ADATE);              /* single conversions, no auto-trigger */
    clear(ADCSRA, ADIE);               /* no interrupt: adc_read() polls     */
    set(ADCSRA, ADEN);                 /* enable the converter               */
}

/* adc_read(): start one conversion on the selected channel and return the
 * 10-bit result (0..1023).  13 ADC clocks = 104 us; the very first
 * conversion after enabling takes 25 clocks.                               */
uint16_t adc_read(void)
{
    set(ADCSRA, ADSC);                 /* start                              */
    while (bit_is_set(ADCSRA, ADSC)) { /* hardware clears ADSC when done     */
    }
    return ADC;                        /* avr-libc reads ADCL then ADCH      */
}

/* adc_read_channel(ch): the multiplexed read.  Re-programs the MUX only
 * when ch differs from the current channel, and throws the first conversion
 * after a switch away so the sample-and-hold capacitor has settled on the
 * new source (no cross-talk between two potentiometers).                   */
uint16_t adc_read_channel(uint8_t ch)
{
    if (ch != g_adc_channel) {
        adc_setup(ch);
        (void)adc_read();
    }
    return adc_read();
}

/* ---------------- test program: print ADC7 (A0) ten times a second ------- */
int main(void)
{
    uint16_t val;

    _clockdivide(0);                   /* 16 MHz */
    m_usb_init();
    adc_setup(7);                      /* PF7 = ADC7 = ItsyBitsy A0 */

    for (;;) {
        val = adc_read();
        m_usb_tx_string("ADC7 = ");
        m_usb_tx_uint(val);
        m_usb_tx_string("\r\n");
        _delay_ms(100);
    }
    return 0;
}
