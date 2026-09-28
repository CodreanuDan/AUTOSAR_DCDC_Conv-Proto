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
    .Ki = 0.15f,       
    .Kd = 0.05f,
    .prev_error = 0.0f,
    .integrator = 0.0f,
    .out_min = 5.0f,    
    .out_max = 98.0f,   
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

    /* Compute Integral term with anti-windup clamping limits */
    pid->integrator += (pid->Ki * error * dt);
    if (pid->integrator > pid->out_max) pid->integrator = pid->out_max;
    if (pid->integrator < pid->out_min) pid->integrator = pid->out_min;

    /* Compute Derivative term based on rate of error change */
    float d_term = 0.0f;
    if (dt > 0.0f) 
    {
        d_term = pid->Kd * (error - pid->prev_error) / dt;
    }
    pid->prev_error = error;

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

    /* 3. Execute PID controller loop if enabled */
    if (pid_disabled == FALSE)
    {
        uint8_t duty_a = 98;
        uint8_t duty_b = 98;

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
            float pid_output_boost = PidCtrl_PIDCompute(&g_PidCtrl_PIDConverter, current_setpoint_volts, v_out, 0.075f);
            Rte_Write_LastPidOutput((uint8_t)pid_output_boost);
            float computed_duty_b = 98.0f - pid_output_boost;
            
            if (computed_duty_b > 98.0f) computed_duty_b = 98.0f;
            if (computed_duty_b < 5.0f) computed_duty_b = 5.0f; 
            
            duty_b = (uint8_t)computed_duty_b;
        }
        /* Condition D: Target setpoint is lower than input voltage, execute Buck control loop */
        else if (current_setpoint_volts < v_in)
        {
            duty_b = 98; 
            float pid_output_buck = PidCtrl_PIDCompute(&g_PidCtrl_PIDConverter, v_out, current_setpoint_volts, 0.075f);
            Rte_Write_LastPidOutput((uint8_t)pid_output_buck);
            float computed_duty_a = 98.0f - pid_output_buck;
            
            if (computed_duty_a > 98.0f) computed_duty_a = 98.0f;
            if (computed_duty_a < 5.0f) computed_duty_a = 5.0f;
            
            duty_a = (uint8_t)computed_duty_a;
        }

        /* 4. Write final computed duty cycles exclusively through RTE ports */
        Rte_Write_DutyA(duty_a);
        Rte_Write_DutyB(duty_b);
    }
}
/************* END OF FUNCTION DEFINITIONS ************/