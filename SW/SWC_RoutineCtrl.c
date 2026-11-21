/*
 * Filename: SWC_RoutineCtrl.c
 * @brief: UDS 0x31 diagnostic test routines. Only one routine runs at a time; a step lasts 500 ms.
 *         All outputs go through the RTE (duty, setpoint, relays), so the safety monitor stays in charge:
 *         a shutdown aborts the routine and the original state is restored.
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "SWC_RoutineCtrl.h"
#include "Rte.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

#define RTN_COUNT        6U
#define RTN_NONE         0xFFU
#define RTN_STEP_TICKS   5U        /* 5 calls of 100 ms = 500 ms per step */
#define RTN_STEPS_SWEEP  10U       /* PWM and PID routines                */
#define RTN_STEPS_RELAY  20U       /* 10 x ON/OFF                         */
#define RTN_DUTY_PASS    98U       /* pass-through duty (bridge fully on) */

enum { IDX_PWM_BOOST = 0U, IDX_PWM_BUCK, IDX_PID_BOOST, IDX_PID_BUCK, IDX_RELAY_IN, IDX_RELAY_OUT };

static uint8_t  s_active     = RTN_NONE;       /* index of the running routine                       */
static uint8_t  s_tick       = 0U;
static uint8_t  s_status[RTN_COUNT];           /* last status of every routine (0 = never run)       */
static uint8_t  s_step[RTN_COUNT];
static uint16_t s_saved_setpoint = 0U;         /* restored when a PID routine ends                   */
static uint8_t  s_saved_relay    = 0U;         /* restored when a relay routine ends                 */
static bool     s_ecum_run       = false;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

static uint8_t Rtn_IndexFromRid(uint16_t rid)
{
    if ((rid >= ROUTINE_RID_PWM_BOOST) && (rid <= ROUTINE_RID_RELAY_OUT))
    {
        return (uint8_t)(rid - ROUTINE_RID_PWM_BOOST);
    }
    return RTN_NONE;
}

/* TRUE for the routines that need the PID controller disabled (they drive the bridge or the relays directly) */
static bool Rtn_NeedsPidDisabled(uint8_t idx)
{
    return (idx == IDX_PWM_BOOST) || (idx == IDX_PWM_BUCK) || (idx == IDX_RELAY_IN) || (idx == IDX_RELAY_OUT);
}

static uint8_t Rtn_StepCount(uint8_t idx)
{
    return ((idx == IDX_RELAY_IN) || (idx == IDX_RELAY_OUT)) ? RTN_STEPS_RELAY : RTN_STEPS_SWEEP;
}

/* Applies one step of the routine through the RTE */
static void Rtn_ApplyStep(uint8_t idx, uint8_t step)
{
    float vin = Rte_Read_Physical_Vin();
    float sp;

    switch (idx)
    {
        case IDX_PWM_BOOST:
            Rte_Write_DutyA(RTN_DUTY_PASS);
            Rte_Write_DutyB((step < 9U) ? (uint8_t)(90U - (10U * step)) : RTN_DUTY_PASS);
            break;

        case IDX_PWM_BUCK:
            Rte_Write_DutyA((step < 9U) ? (uint8_t)(90U - (10U * step)) : RTN_DUTY_PASS);
            Rte_Write_DutyB(RTN_DUTY_PASS);
            break;

        case IDX_PID_BOOST:
            sp = vin + ((step < 9U) ? (2.0f * (float)(step + 1U)) : 0.0f);
            Rte_Write_PidTargetSetpoint((uint16_t)(sp + 0.5f));
            break;

        case IDX_PID_BUCK:
            sp = vin - ((step < 9U) ? (float)(step + 1U) : 0.0f);
            if (sp < 0.0f) { sp = 0.0f; }
            Rte_Write_PidTargetSetpoint((uint16_t)(sp + 0.5f));
            break;

        case IDX_RELAY_IN:
            Rte_Call_RelayInput_Set(((step & 1U) == 0U) ? 1U : 0U);      /* even step ON, odd step OFF */
            break;

        case IDX_RELAY_OUT:
            Rte_Call_RelayOutput_Set(((step & 1U) == 0U) ? 1U : 0U);
            break;

        default:
            break;
    }
}

/* Ends the active routine, restores the state it changed and records the result */
static void Rtn_Finish(uint8_t status)
{
    uint8_t idx = s_active;

    if (idx == RTN_NONE)
    {
        return;
    }

    switch (idx)
    {
        case IDX_PWM_BOOST:
        case IDX_PWM_BUCK:
            Rte_Write_DutyA(RTN_DUTY_PASS);
            Rte_Write_DutyB(RTN_DUTY_PASS);
            break;
        case IDX_PID_BOOST:
        case IDX_PID_BUCK:
            Rte_Write_PidTargetSetpoint(s_saved_setpoint);
            break;
        case IDX_RELAY_IN:
            Rte_Call_RelayInput_Set(s_saved_relay);
            break;
        case IDX_RELAY_OUT:
            Rte_Call_RelayOutput_Set(s_saved_relay);
            break;
        default:
            break;
    }

    s_status[idx] = status;
    s_active = RTN_NONE;
    s_tick = 0U;
    Rte_Write_RoutineActive(0U);
}

bool SWC_RoutineCtrl_IsKnown(uint16_t rid)
{
    return (Rtn_IndexFromRid(rid) != RTN_NONE);
}

uint8_t SWC_RoutineCtrl_Start(uint16_t rid)
{
    uint8_t idx = Rtn_IndexFromRid(rid);

    if (idx == RTN_NONE)
    {
        return 0x31U;                                   /* requestOutOfRange */
    }

    /* Preconditions: nothing else running, ECU in RUN, converter not shut down, PID in the mode the routine needs */
    if ((s_active != RTN_NONE) || (s_ecum_run == false) || (Rte_Read_ConverterEnable() == 0U))
    {
        return 0x22U;                                   /* conditionsNotCorrect */
    }
    if (Rtn_NeedsPidDisabled(idx) != (Rte_Read_PidDisableFlag() != false))
    {
        return 0x22U;
    }

    s_saved_setpoint = Rte_Read_PidTargetSetpoint();
    if (idx == IDX_RELAY_IN)  { s_saved_relay = Rte_Read_RelayInputRequest(); }
    if (idx == IDX_RELAY_OUT) { s_saved_relay = Rte_Read_RelayOutputRequest(); }

    s_active = idx;
    s_tick = 0U;
    s_step[idx] = 0U;
    s_status[idx] = ROUTINE_STATUS_RUNNING;
    Rte_Write_RoutineActive((uint8_t)(idx + 1U));
    Rtn_ApplyStep(idx, 0U);                             /* step 0 starts immediately */
    return 0U;
}

uint8_t SWC_RoutineCtrl_Stop(uint16_t rid)
{
    uint8_t idx = Rtn_IndexFromRid(rid);

    if (idx == RTN_NONE)
    {
        return 0x31U;
    }
    if (s_active == idx)
    {
        Rtn_Finish(ROUTINE_STATUS_STOPPED);
    }
    return 0U;
}

uint8_t SWC_RoutineCtrl_GetResult(uint16_t rid, uint8_t *status, uint8_t *step)
{
    uint8_t idx = Rtn_IndexFromRid(rid);

    if (idx == RTN_NONE)
    {
        return 0x31U;
    }
    *status = s_status[idx];
    *step   = s_step[idx];
    return 0U;
}

void SWC_RoutineCtrl_Abort(void)
{
    Rtn_Finish(ROUTINE_STATUS_ABORTED);
}

void SWC_RoutineCtrl_Runnable(bool ecum_run)
{
    uint8_t idx = s_active;

    s_ecum_run = ecum_run;
    if (idx == RTN_NONE)
    {
        return;
    }

    /* Abort when the conditions of the start are lost: ECU left RUN, safety shutdown, PID mode changed by the tester */
    if ((ecum_run == false) || (Rte_Read_ConverterEnable() == 0U) ||
        (Rtn_NeedsPidDisabled(idx) != (Rte_Read_PidDisableFlag() != false)))
    {
        Rtn_Finish(ROUTINE_STATUS_ABORTED);
        return;
    }

    s_tick++;
    if (s_tick < RTN_STEP_TICKS)
    {
        return;
    }
    s_tick = 0U;

    s_step[idx]++;
    if (s_step[idx] >= Rtn_StepCount(idx))
    {
        Rtn_Finish(ROUTINE_STATUS_COMPLETED);
    }
    else
    {
        Rtn_ApplyStep(idx, s_step[idx]);
    }
}
