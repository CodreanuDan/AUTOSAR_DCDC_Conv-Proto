/*
 * Filename: SWC_PidController.c
 * @brief: Contains PID Controller implementation, converter state logic, and RTE communication ports
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "SWC_PidController.h"
#include "Rte.h"
#include "StdTypes.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

#define PID_DT_S        0.01f    /* real task period: 10 ms */

#define BOOST_FF_GAIN         1.0f     /* start at 1.0 (ideal), raise toward ~1.5 after calibration */
#define BOOST_OUT_MIN   0.0f
#define BOOST_OUT_MAX   85.0f    /* below the collapse point; tune from your plot */

#define BUCK_FF_GAIN    1.0f     /* calibrate separately from boost */
#define BUCK_OUT_MIN    0.0f
#define BUCK_OUT_MAX    93.0f    /* duty_a never below 5 % */

typedef struct {
    float Kp;           
    float Ki;           
    float Kd;           
    float prev_error;   
    float integrator;   
    float out_min;      
    float out_max;      
    float deadzone;     
} PID_Controller;

/* Static configuration instance for the DCDC Converter PID loop */
static PID_Controller g_PidCtrl_PIDConverter = {
    .Kp = 0.85f,
    .Ki = 1.0f,          /* was 0.15 with dt=0.1; per-second equivalent ~1.5, start lower */
    .Kd = 0.0f,          /* off for now: Kd/dt amplifies ADC noise at dt = 0.01 */
    .prev_error = 0.0f,
    .integrator = 0.0f,
    .out_min = -15.0f,   /* trim limits, NOT the duty limits */
    .out_max =  15.0f,
    .deadzone = 0.2f
};

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

/*
 * Function name: PidCtrl_PIDCompute
 * @brief Core PID mathematical algorithm execution with deadzone and anti-windup clamping.
 * @param: PID_Controller *pid, float setpoint, float feedback, float dt
 * @return: float computed output value
 */
static float PidCtrl_PIDCompute(PID_Controller *pid, float setpoint, float feedback, float dt) 
{
    /* Calculate error between target setpoint and current feedback */
    float error = setpoint - feedback;

    /* Apply deadzone filter to ignore minor perturbations */
    if (error > -pid->deadzone && error < pid->deadzone) 
    {
        error = 0.0f;
    }
	
	/* Compute Proportional term */
	float p_term = pid->Kp * error;

	/* Compute Derivative term based on rate of error change */
	float d_term = 0.0f;
	if (dt > 0.0f)
	{
		d_term = pid->Kd * (error - pid->prev_error) / dt;
	}
	pid->prev_error = error;

	/* Compute Integral term with conditional-integration anti-windup:
	 * only accumulate while the unclamped output isn't already saturated,
	 * so the integrator can't wind up past what the actuator can use. */
	float output_unclamped = p_term + pid->integrator + d_term;
	bool saturated_high = (output_unclamped >= pid->out_max) && (error > 0.0f);
	bool saturated_low  = (output_unclamped <= pid->out_min) && (error < 0.0f);

	if (!saturated_high && !saturated_low)
	{
		pid->integrator += (pid->Ki * error * dt);
	}

    /* Sum all terms and clamp total output within configured boundaries */
    float output = p_term + pid->integrator + d_term;
    if (output > pid->out_max) output = pid->out_max;
    if (output < pid->out_min) output = pid->out_min;

    return output;
}

/*
 * Function name: SWC_PidController_Runnable
 * @brief Evaluates converter operating state and executes PID control loops via RTE interfaces.
 * @param: float v_in, float v_out
 * @return: void
 */
void SWC_PidController_Runnable(float v_in, float v_out)
{
    /* 1. Evaluate and write converter operating state via RTE */
    if (v_out >= (v_in - 0.3f) && v_out <= (v_in + 0.3f))
    {
        Rte_Write_ConverterState(CONVERTER_STATE_PASSTHROUGH);
    }
    else if (v_out < v_in)
    {
        Rte_Write_ConverterState(CONVERTER_STATE_BOOST);
    }
    else
    {
        Rte_Write_ConverterState(CONVERTER_STATE_BUCK);
    }

    /* 2. Read configuration flags and target setpoint via RTE */
    bool pid_disabled = Rte_Read_PidDisableFlag();
    uint16_t target_setpoint = Rte_Read_PidTargetSetpoint();
    float current_setpoint_volts = (float)target_setpoint;
    static uint8_t s_prev_mode = 0U;            /* 0 none, 1 boost, 2 buck */

    /* 3. Execute PID controller loop if enabled */
    if (pid_disabled == FALSE)
    {
        uint8_t duty_a = 98;
        uint8_t duty_b = 98;

        /* Reset the PID state whenever the control mode changes */
        uint8_t mode = (current_setpoint_volts > v_in) ? 1U : 2U;
        if (mode != s_prev_mode)
        {
            g_PidCtrl_PIDConverter.integrator = 0.0f;
            g_PidCtrl_PIDConverter.prev_error = 0.0f;
            s_prev_mode = mode;
        }

        /* Condition A: Output voltage is critically low, bypass control and saturate duty cycles */
        if (v_out < 2.0f) 
        {
            duty_a = 98;
            duty_b = 98;
            g_PidCtrl_PIDConverter.integrator = 0.0f;
            g_PidCtrl_PIDConverter.prev_error = 0.0f;
        }
        /* Condition B: Target setpoint matches input voltage range, run in pass-through mode */
        else if (current_setpoint_volts >= (v_in - 0.3f) && current_setpoint_volts <= (v_in + 0.3f))
        {
            duty_a = 98;
            duty_b = 98;
            g_PidCtrl_PIDConverter.integrator = 0.0f; 
            g_PidCtrl_PIDConverter.prev_error = 0.0f;
        }
        /* Condition C: Target setpoint is higher than input voltage, execute Boost control loop */
		else if (current_setpoint_volts > v_in)
		{
			duty_a = 98;

			/* Feed-forward: ideal boost duty for this Vin/setpoint, in "output" units */
			float ff   = BOOST_FF_GAIN * 98.0f * (1.0f - (v_in / current_setpoint_volts));

			/* PID trim: small correction, limited to +/-15 by the PID struct */
			float trim = PidCtrl_PIDCompute(&g_PidCtrl_PIDConverter, current_setpoint_volts, v_out, PID_DT_S);

			float out = ff + trim;
			if (out > BOOST_OUT_MAX) out = BOOST_OUT_MAX;
			if (out < BOOST_OUT_MIN) out = BOOST_OUT_MIN;

			Rte_Write_LastPidOutput((uint8_t)out);     /* total output, not the trim (trim can be negative) */

			float computed_duty_b = 98.0f - out;
			if (computed_duty_b > 98.0f) computed_duty_b = 98.0f;
			if (computed_duty_b < 5.0f)  computed_duty_b = 5.0f;

			duty_b = (uint8_t)computed_duty_b;
		}
        /* Condition D: Target setpoint is lower than input voltage, execute Buck control loop */
		else if (current_setpoint_volts < v_in)
		{
			duty_b = 98;

			float ff   = BUCK_FF_GAIN * 98.0f * (1.0f - (current_setpoint_volts / v_in));
			float trim = PidCtrl_PIDCompute(&g_PidCtrl_PIDConverter, v_out, current_setpoint_volts, PID_DT_S);  /* note: swapped */

			float out = ff + trim;
			if (out > BUCK_OUT_MAX) out = BUCK_OUT_MAX;
			if (out < BUCK_OUT_MIN) out = BUCK_OUT_MIN;

			Rte_Write_LastPidOutput((uint8_t)out);

			float computed_duty_a = 98.0f - out;
			if (computed_duty_a > 98.0f) computed_duty_a = 98.0f;
			if (computed_duty_a < 5.0f)  computed_duty_a = 5.0f;

			duty_a = (uint8_t)computed_duty_a;
		}

        /* 4. Write final computed duty cycles exclusively through RTE ports */
        Rte_Write_DutyA(duty_a);
        Rte_Write_DutyB(duty_b);
    }
    else
    {
        s_prev_mode = 0U;   /* PID disabled: force a clean start when it is re-enabled */
    }
}
/************* END OF FUNCTION DEFINITIONS ************/