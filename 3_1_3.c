/*************************************************************************
 * MEAM 5100 Lab 3.1.3 - Master (input) arm: read both joint potentiometers
 * and print the joint angles on the USB serial port.
 * Linqi Zhe
 *
 * Planar 2-DOF master arm (3.1.1 design document):
 *   joint 1 = shoulder : Bourns 3310 pot, wiper on PF7/ADC7 (ItsyBitsy A0)
 *   joint 2 = elbow    : Bourns 3310 pot, wiper on PF6/ADC6 (ItsyBitsy A1)
 * The pots have 270 deg of electrical travel and are clamped to the links
 * at mid-travel with the link straight ahead, so
 *   theta = (adc - 512) * 270 / 1023   [deg; +-90 deg are the working range]
 * One line every 100 ms, e.g.   th1=+012.3deg (adc 559)   th2=-045.0deg (adc 341)
 *************************************************************************/
#include "MEAM_general.h"
#include "m_usb.h"

#define SHOULDER_CH      7       /* PF7 / ADC7 / A0 */
#define ELBOW_CH         6       /* PF6 / ADC6 / A1 */
#define POT_TRAVEL_DEG   270     /* Bourns 3310: effective electrical angle */
#define ADC_CENTRE       512     /* reading with the link straight ahead     */
#define PRINT_PERIOD_MS  100     /* 10 Hz: readable in the video             */

/* ------------- ADC subroutines (identical to 3_1_2.c) ------------------- */
static uint8_t g_adc_channel = 0;

static void adc_disable_digital_input(uint8_t ch)
{
    if (ch <= 7)       { set(DIDR0, ch); }
    else if (ch <= 13) { set(DIDR2, (ch - 8)); }
}

void adc_setup(uint8_t ch)
{
    if (ch > 13) ch = 0;
    g_adc_channel = ch;
    clear(ADMUX, REFS1);  set(ADMUX, REFS0);        /* AVcc reference        */
    clear(ADMUX, ADLAR);                            /* 10-bit, right-adjusted */
    clear(ADMUX, MUX0); clear(ADMUX, MUX1); clear(ADMUX, MUX2); clear(ADMUX, MUX3);
    clear(ADCSRB, MUX5);
    if (ch <= 7) {
        if (ch & 0x01) set(ADMUX, MUX0);
        if (ch & 0x02) set(ADMUX, MUX1);
        if (ch & 0x04) set(ADMUX, MUX2);
    } else {
        uint8_t low = (uint8_t)(ch - 8);
        set(ADCSRB, MUX5);
        if (low & 0x01) set(ADMUX, MUX0);
        if (low & 0x02) set(ADMUX, MUX1);
        if (low & 0x04) set(ADMUX, MUX2);
    }
    adc_disable_digital_input(ch);
    set(ADCSRA, ADPS2); set(ADCSRA, ADPS1); set(ADCSRA, ADPS0);   /* clk/128 */
    clear(ADCSRA, ADATE);
    clear(ADCSRA, ADIE);
    set(ADCSRA, ADEN);
}

uint16_t adc_read(void)
{
    set(ADCSRA, ADSC);
    while (bit_is_set(ADCSRA, ADSC)) { }
    return ADC;
}

uint16_t adc_read_channel(uint8_t ch)
{
    if (ch != g_adc_channel) {
        adc_setup(ch);
        (void)adc_read();                           /* settle after the switch */
    }
    return adc_read();
}

/* ------------- angle conversion and printing ---------------------------- */
/* joint angle in tenths of a degree, signed: (adc - 512) * 2700 / 1023 */
static int angle_tenths(uint16_t adc)
{
    long d = (long)adc - ADC_CENTRE;
    return (int)((d * (10L * POT_TRAVEL_DEG)) / 1023L);
}

/* print the angle as +ddd.d */
static void print_angle(int tenths)
{
    unsigned int mag;
    if (tenths < 0) { m_usb_tx_char('-'); mag = (unsigned int)(-tenths); }
    else            { m_usb_tx_char('+'); mag = (unsigned int)tenths;    }
    m_usb_tx_uint(mag / 10);
    m_usb_tx_char('.');
    m_usb_tx_char((char)('0' + (mag % 10)));
}

int main(void)
{
    uint16_t adc1, adc2;

    _clockdivide(0);                                /* 16 MHz                 */
    m_usb_init();
    while (!m_usb_isconnected()) { }                /* wait for the terminal  */
    _delay_ms(500);
    adc_setup(SHOULDER_CH);

    for (;;) {
        adc1 = adc_read_channel(SHOULDER_CH);
        adc2 = adc_read_channel(ELBOW_CH);

        m_usb_tx_string("th1=");   print_angle(angle_tenths(adc1));
        m_usb_tx_string("deg (adc "); m_usb_tx_uint(adc1); m_usb_tx_string(")   ");
        m_usb_tx_string("th2=");   print_angle(angle_tenths(adc2));
        m_usb_tx_string("deg (adc "); m_usb_tx_uint(adc2); m_usb_tx_string(")\r\n");
        m_usb_tx_push();

        _delay_ms(PRINT_PERIOD_MS);
    }
    return 0;
}
