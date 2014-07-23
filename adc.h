
/** ADC code for Brainlink, see adc.c for implementation details. */


// Defines the position of each analog port for use with read_analog
#define LIGHT      ADC_CH_MUXPOS_PIN6_gc
#define BATT_VOLT  ADC_CH_MUXPOS_PIN7_gc
#define AUX0       ADC_CH_MUXPOS_PIN0_gc
#define AUX1       ADC_CH_MUXPOS_PIN1_gc
#define AUX2       ADC_CH_MUXPOS_PIN2_gc
#define AUX3       ADC_CH_MUXPOS_PIN3_gc
#define AUX4       ADC_CH_MUXPOS_PIN4_gc
#define AUX5       ADC_CH_MUXPOS_PIN5_gc

#define START_CH2 0b00010000;
#define START_CH1 0b00001000;
#define START_CH0 0b00000100;
#define START_CH3 0b00100000;
#define START_CH  0b10000000;

/*! \brief This macro enables the selected adc.
 *
 *  Before the ADC is enabled the first time the function
 *  ADC_CalibrationValues_Set should be used to reduce the gain error in the
 *  ADC.
 *
 *  \note After the ADC is enabled the commen mode voltage in the ADC is ready
 *        after 12 ADC clock cycels. Do one dummy conversion or wait the required
 *        number of clock cycles to reasure correct conversion.
 *
 *  \param  _adc          Pointer to ADC module register section.
 */
#define ADC_Enable(_adc) ((_adc)->CTRLA |= ADC_ENABLE_bm)

/*! \brief This macro disables the selected adc.
 *
 *  \param  _adc  Pointer to ADC module register section
 */
#define ADC_Disable(_adc) ((_adc)->CTRLA = (_adc)->CTRLA & (~ADC_ENABLE_bm))

/*! \brief This macro set the conversion mode and resolution in the selected adc.
 *
 *  This macro configures the conversion mode to signed or unsigned and set
 *  the resolution in and the way the results are put in the result
 *  registers.
 *
 *  \param  _adc          Pointer to ADC module register section
 *  \param  _signedMode   Selects conversion mode: signed (true)
 *                        or unsigned (false). USE bool type.
 *  \param  _resolution   Resolution and presentation selection.
 *                        Use ADC_RESOLUTION_t type.
 */

#define ADC_ConvMode_and_Resolution_Config(_adc, _signedMode, _resolution)     \
	((_adc)->CTRLB = ((_adc)->CTRLB & (~(ADC_RESOLUTION_gm|ADC_CONMODE_bm)))|  \
		(_resolution| ( _signedMode? ADC_CONMODE_bm : 0)))
/*! \brief Helper macro for increased readability with ADC_ConvMode_and_Resolution_Config
 *
 *  \sa  ADC_ConvMode_and_Resolution_Config
 */
#define ADC_ConvMode_Signed true

/*! \brief Helper macro for increased readability with ADC_ConvMode_and_Resolution_Config
 *
 *  \sa  ADC_ConvMode_and_Resolution_Config
 */
#define ADC_ConvMode_Unsigned false

/*! \brief This macro set the prescaler factor in the selected adc.
 *
 *  This macro configures the division factor between the XMEGA
 *  IO-clock and the ADC clock. Given a certain IO-clock, the prescaler
 *  must be configured so the the ADC clock is within recommended limits.
 *  A faster IO-clock required higher division factors.
 *
 *  \note  The maximum ADC sample rate is always one fourth of the IO clock.
 *
 *  \param  _adc  Pointer to ADC module register section.
 *  \param  _div  ADC prescaler division factor setting. Use ADC_PRESCALER_t type
 */
#define ADC_Prescaler_Config(_adc, _div)                                       \
	((_adc)->PRESCALER = ((_adc)->PRESCALER & (~ADC_PRESCALER_gm)) | _div)
	

/*! \brief This macro set the conversion reference in the selected adc.
 *
 *  \param  _adc      Pointer to ADC module register section.
 *  \param  _convRef  Selects reference voltage for all conversions.
 *                    Use ADC_REFSEL_t type.
 */
#define ADC_Reference_Config(_adc, _convRef)                                   \
	((_adc)->REFCTRL = ((_adc)->REFCTRL & ~(ADC_REFSEL_gm)) | _convRef)


/*! \brief This macro sets the sweep channel settings.
 *
 *  \param  _adc            Pointer to ADC module register section.
 *  \param  _sweepChannels  Sweep channel selection. Use ADC_SWEEP_t type
 */
#define ADC_SweepChannels_Config(_adc, _sweepChannels)                         \
	((_adc)->EVCTRL = ((_adc)->EVCTRL & (~ADC_SWEEP_gm)) | _sweepChannels)


/*! \brief This macro configures the event channels used and the event mode.
 *
 *  This macro configures the way events are used to trigger conversions for
 *  the virtual channels. Use the eventChannels parameter to select which event
 *  channel to associate with virtual channel 0 or to trigger a conversion sweep,
 *  depending on the selected eventMode parameter.
 *
 *  \param  _adc            Pointer to ADC module register section.
 *  \param  _eventChannels  The first event channel to be used for triggering.
 *                          Use ADC_EVSEL_t type.
 *  \param  _eventMode      Select event trigger mode.
 *                          Use ADC_EVACT_t type.
 */
#define ADC_Events_Config(_adc, _eventChannels, _eventMode)                    \                                                                           \
	(_adc)->EVCTRL = ((_adc)->EVCTRL & (~(ADC_EVSEL_gm | ADC_EVACT_gm))) | \
	                 ((uint8_t) _eventChannels | _eventMode)

/*! \brief This macro configures the input mode and gain to a specific virtual channel.
 *
 *  \param  _adc_ch         Pointer to ADC channel register section.
 *  \param  _inputMode      Input mode for this channel, differential,
 *                         single-ended, gain etc. Use ADC_CH_INPUTMODE_t type.
 *  \param  _gain           The preamplifiers gain value.
 *                         Use ADC_CH_GAINFAC_t type.
 *
 */
#define ADC_Ch_InputMode_and_Gain_Config(_adc_ch, _inputMode, _gain)           \
	(_adc_ch)->CTRL = ((_adc_ch)->CTRL &                                   \
	                  (~(ADC_CH_INPUTMODE_gm|ADC_CH_GAINFAC_gm))) |        \
	                  ((uint8_t) _inputMode|_gain)

/*! \brief Helper macro for increased readability with ADC_Ch_InputMode_and_Gain_Config
 *
 *  \sa  ADC_Ch_InputMode_and_Gain_Config
 */
#define ADC_DRIVER_CH_GAIN_NONE ADC_CH_GAIN_1X_gc

/*!  \brief This macro configures the Positiv and negativ inputs.
 *
 *  \param  _adc_ch    Which ADC channel to configure.
 *  \param  _posInput  Which pin (or internal signal) to connect to positive
 *                     ADC input. Use ADC_CH_MUXPOS_enum type.
 *  \param  _negInput  Which pin to connect to negative ADC input.
 *                     Use ADC_CH_MUXNEG_t type.
 *
 *  \note  The negative input is connected to GND for single-ended and internal input modes.
 */
#define ADC_Ch_InputMux_Config(_adc_ch, _posInput, _negInput)                  \
	((_adc_ch)->MUXCTRL = (uint8_t) _posInput | _negInput)


/*! \brief This macro enables the Free Running mode in the selected adc.
 *
 *  \param  _adc   Pointer to ADC module register section.
 */
#define ADC_FreeRunning_Enable(_adc)  ((_adc)->CTRLB |= ADC_FREERUN_bm)


/*! \brief This macro disables the Free Running mode in the selected adc.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_FreeRunning_Disable(_adc)                                          \
	((_adc)->CTRLB = (_adc)->CTRLB & (~ADC_FREERUN_bm))


/*! \brief This macro makes sure that the temperature reference circuitry is enabled.
 *
 *  \note  Enabling the temperature reference automatically enables the bandgap reference.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_TempReference_Enable(_adc) ((_adc)->REFCTRL |= ADC_TEMPREF_bm)


/*! \brief This macro disables the temperature reference.
 *
 *  \param  _adc  Pointer to ADC module register section.
 */
#define ADC_TempReference_Disable(_adc)                                        \
	((_adc)->REFCTRL = (_adc)->REFCTRL & (~ADC_TEMPREF_bm))

/*! \brief This macro returns the channel conversion complete flag..
 *
 *  \param  _adc_ch  Pointer to ADC Channel register section.
 *
 *  \return value of channels conversion complete flag.
 */
#define ADC_Ch_Conversion_Complete(_adc_ch)                                    \
	(((_adc_ch)->INTFLAGS & ADC_CH_CHIF_bm) != 0x00)
	

void init_adc(void);
int read_analog(ADC_CH_MUXPOS_t sensor);
int read_internal_temperature(void);