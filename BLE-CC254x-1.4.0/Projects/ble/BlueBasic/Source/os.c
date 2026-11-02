////////////////////////////////////////////////////////////////////////////////
// BlueBasic
////////////////////////////////////////////////////////////////////////////////
//
// Authors:
//      Tim Wilkinson <timjwilkinson@gmail.com>
//
// os.c
//

#include "os.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "hal_uart.h"
#include "OSAL_PwrMgr.h"
#ifdef FEATURE_OAD_HEADER
#include "oad.h"
#include "oad_target.h"
#include "hal_flash.h"
#endif
#include "hal_uart.h"
#include "hal_mcu.h"

#define FILE_HANDLE_PROGRAM     0
#define FILE_HANDLE_DATA        1
#define FILE_HANDLE_MAX         (OS_MAX_FILE)

#define	FILE_HEADER             (0x80)
#define FILE_PROGRAM_FIRST      (0x81)
#define FILE_PROGRAM_END        (FILE_PROGRAM_FIRST+0x20)
#define FILE_DATA_FIRST         (FILE_PROGRAM_END+1)
#define FILE_BLOCK_SIZE         (128)

extern void osalTimeUpdate(void);

extern uint8 blueBasic_TaskID;

os_interrupt_t blueBasic_interrupts[OS_MAX_INTERRUPT];
os_timer_t blueBasic_timers[OS_MAX_TIMER];
os_discover_t blueBasic_discover;

struct program_header
{
  char autorun;
  unsigned short length[OS_MAX_FILE];
};

// Timers
#define NR_TIMERS ((OS_MAX_TIMER) - 1)
struct
{
  unsigned short lineno;
} timers[OS_MAX_TIMER];

// Serial
os_serial_t serial[OS_MAX_SERIAL];

enum {
  MODE_STARTUP = 0,
  MODE_NEED_INPUT,
  MODE_GOT_INPUT,
  MODE_RUNNING

};
static struct {
  char* start;
  char* end;
  char* ptr;
  char mode;
  char quote;
} input;

#ifdef ENABLE_BLE_CONSOLE

extern unsigned char ble_console_write(unsigned char ch);
extern unsigned char ble_console_enabled;

#endif

#define CR	'\r'
#define NL	'\n'
#define BELL	'\b'
#define SQUOTE  '\''
#define DQUOTE  '\"'
#define CTRLC	0x03
#define CTRLH	0x08

void OS_power_saving(unsigned char enable)
{
#if defined ( POWER_SAVING )
  if (enable)
  {
    osal_pwrmgr_device(PWRMGR_BATTERY);
    osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_SLEEP_EVT, 100);
  }
  else
  {
    osal_stop_timerEx(blueBasic_TaskID, BLUEBASIC_SLEEP_EVT);
    osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
    osal_pwrmgr_task_state(blueBasic_TaskID, PWRMGR_HOLD);
  }
#endif
}

volatile uint8 serial_waking = 0;

void OS_prompt_reset(void)
{
  input.ptr = input.start;
  input.quote = 0;
}

void OS_type(char c)
{
  static char last_was_cr = 0;

  // Silently drop any desynchronized byte received during crystal spin-up
  if (serial_waking)
  {
    return;
  }

  // Handle CRLF: If previous char was CR and current is NL, drop the redundant NL
  if (c == NL && last_was_cr)
  {
    last_was_cr = 0;
    return;
  }
  last_was_cr = (c == CR);

  // Filter out framing errors (0x00), bytes with MSB set, and non-printable control characters
  // except CR, NL, Backspace (CTRLH), DEL (0x7F), and Tab (0x09)
  if ((unsigned char)c == 0x00 || (unsigned char)c > 0x7E)
  {
    return;
  }
  if ((unsigned char)c < 0x20 && c != NL && c != CR && c != CTRLH && c != '\t')
  {
    return;
  }

#if defined ( POWER_SAVING )
  osal_pwrmgr_task_state(blueBasic_TaskID, PWRMGR_HOLD);
#endif
  switch (c)
  {
    case 0xff:
      break;
    case NL:
    case CR:
#ifdef ENABLE_CONSOLE_ECHO
      OS_putchar('\n');
#endif // ENABLE_CONSOLE_ECHO
      if (input.mode == MODE_NEED_INPUT)
      {
        *input.ptr = NL;
        input.mode = MODE_GOT_INPUT;
        // Wake the interpreter now we have something new to do
        osal_set_event(blueBasic_TaskID, BLUEBASIC_INPUT_AVAILABLE);
      }
      break;
    case CTRLH:
    case 0x7F:
      if (input.mode != MODE_NEED_INPUT)
      {
        break;
      }
      if(input.ptr == input.start)
      {
        break;
      }
      if (*--input.ptr == input.quote)
      {
        input.quote = 0;
      }
#ifdef ENABLE_CONSOLE_ECHO
      OS_putchar(CTRLH);
      OS_putchar(' ');
      OS_putchar(CTRLH);
#endif // ENABLE_CONSOLE_ECHO
      break;
    default:
      if (input.mode != MODE_NEED_INPUT)
      {
        break;
      }
      if(input.ptr != input.end)
      {
        // Are we in a quoted string?
        if(c == input.quote)
        {
          input.quote = 0;
        }
        else if (c == DQUOTE || c == SQUOTE)
        {
          input.quote = c;
        }
        else if (input.quote == 0 && c >= 'a' && c <= 'z')
        {
          c = c + 'A' - 'a';
        }
        *input.ptr++ = c;
#ifdef ENABLE_CONSOLE_ECHO
        OS_putchar(c);
#endif
      }
      break;
  }
}

void OS_prompt_buffer(unsigned char* start, unsigned char* end)
{
  input.mode = MODE_NEED_INPUT;
  input.start = (char*)start;
  input.end = (char*)end;
  input.ptr = (char*)start;
  input.quote = 0;
}

char OS_prompt_available(void)
{
  if (input.mode == MODE_GOT_INPUT)
  {
    input.mode = MODE_RUNNING;
    return 1;
  }
  else
  {
    return 0;
  }
}

void OS_putchar(char ch)
{
#ifdef ENABLE_BLE_CONSOLE
  ble_console_write(ch);
#endif
#ifdef OS_UART_PORT
  if (ch == '\n')
  {
    uint8 cr = '\r';
    while (HalUARTWrite(OS_UART_PORT, &cr, 1) == 0)
      ;
  }
  while (HalUARTWrite(OS_UART_PORT, (uint8*)&ch, 1) == 0)
    ;
#endif
}

void* OS_rmemcpy(void *dst, const void GENERIC *src, unsigned int len)
{
  uint8 *pDst;
  const uint8 GENERIC *pSrc;

  pSrc = len + (uint8*)src;
  pDst = len + (uint8*)dst;

  while ( len-- )
    *--pDst = *--pSrc;

  return ( pDst );
}

void OS_init(void)
{
  OS_servo_init();
  OS_motor_init();
  OS_openserial();
}

void OS_timer_stop(unsigned char id)
{
  blueBasic_timers[id].linenum = 0;
  osal_stop_timerEx(blueBasic_TaskID, BLUEBASIC_EVENT_TIMER << id);
}

char OS_timer_start(unsigned char id, unsigned long timeout, unsigned char repeat, unsigned short linenum)
{
  if (id >= OS_MAX_TIMER)
  {
    return 0;
  }
  blueBasic_timers[id].linenum = linenum;
  if (repeat)
  {
    osal_start_reload_timer(blueBasic_TaskID, BLUEBASIC_EVENT_TIMER << id, timeout);
  }
  else
  {
    osal_start_timerEx(blueBasic_TaskID, BLUEBASIC_EVENT_TIMER << id, timeout);
  }
  return 1;
}

char OS_interrupt_attach(unsigned char pin, unsigned short lineno, unsigned short debounce_ms, unsigned char falling)
{
  unsigned char i;

  for (i = 0; i < OS_MAX_INTERRUPT; i++)
  {
    if (blueBasic_interrupts[i].linenum == 0 || blueBasic_interrupts[i].pin == pin)
    {
      blueBasic_interrupts[i].pin = pin;
      blueBasic_interrupts[i].linenum = lineno;
      blueBasic_interrupts[i].debounce_ms = debounce_ms;
      blueBasic_interrupts[i].falling = falling;
      blueBasic_interrupts[i].debouncing = 0;
      return 1;
    }
  }
  return 0;
}

char OS_interrupt_detach(unsigned char pin)
{
  unsigned char i;

  for (i = 0; i < OS_MAX_INTERRUPT; i++)
  {
    if (blueBasic_interrupts[i].pin == pin)
    {
      blueBasic_interrupts[i].pin = 0;
      blueBasic_interrupts[i].linenum = 0;
      blueBasic_interrupts[i].debounce_ms = 0;
      blueBasic_interrupts[i].falling = 0;
      blueBasic_interrupts[i].debouncing = 0;
      return 1;
    }
  }
  return 0;
}

long OS_get_millis(void)
{
  osalTimeUpdate();
  return osal_getClock() * 1000 + osal_getMSClock();
}

void OS_set_millis(long timems)
{
  osal_setClock(timems / 1000);
}

void OS_reboot(char flash)
{
 #ifdef FEATURE_OAD_HEADER
  if (flash)
  {
    short zero = 0;
    uint16 addr = OAD_IMG_B_PAGE * (HAL_FLASH_PAGE_SIZE / HAL_FLASH_WORD_SIZE);
    HalFlashWrite(addr, (uint8*)&zero, sizeof(zero));
  }
#else
  VOID flash;
#endif
  SystemReset();
}

#pragma optimize=none
void OS_delaymicroseconds(short micros)
{
  // An empty loop is pretty accurate it turns out
  while (micros-- > 0)
    ;
}

void OS_flashstore_init(void)
{
  // If flashstore is uninitialized, deleting all the pages will set it up correctly.
  if (*(unsigned long*)FLASHSTORE_CPU_BASEADDR == 0xFFFFFFFF)
  {
    flashstore_deleteall();
  }
}

static void _uartCallback(uint8 port, uint8 event)
{
#ifdef HAL_UART_RX_WAKEUP
  // UART woken up. This happens in the interrupt handler so we really
  // don't want to do anything else.
  if (event == HAL_UART_RX_WAKEUP)
  {
    return;
  }
#endif
#ifdef OS_UART_PORT
  if (port == OS_UART_PORT)
  {
    if (serial[0].onread || serial[0].onwrite)
    {
      osal_set_event(blueBasic_TaskID, BLUEBASIC_EVENT_SERIAL);
    }
    else
    {
      uint8 ch;
      while (HalUARTRead(OS_UART_PORT, &ch, 1) > 0)
      {
        OS_type((char)ch);
      }
    }
  }
#endif
}

void OS_openserial(void)
{
  OS_serial_open(0, 9600, 'N', 8, 1, 'N', 0, 0);
}

unsigned char OS_serial_open(unsigned char port, unsigned long baud, unsigned char parity, unsigned char bits, unsigned char stop, unsigned char flow, unsigned short onread, unsigned short onwrite)
{
  halUARTCfg_t config;
  
  switch (baud)
  {
    case 9600:
      baud = HAL_UART_BR_9600;
      break;
    case 19200:
      baud = HAL_UART_BR_19200;
      break;
    case 38400:
      baud = HAL_UART_BR_38400;
      break;
    case 57600:
      baud = HAL_UART_BR_57600;
      break;
    case 115200:
      baud = HAL_UART_BR_115200;
      break;
    default:
      return 2;
  }
 
  // Support port 0, no-parity, 8-bits, 1 stop bit, flow 'N' or 'H'
  if (port != 0 || parity != 'N' || bits != 8 || stop != 1 || (flow != 'N' && flow != 'H'))
  {
    return 3;
  }

  config.configured = 1;
  config.baudRate = baud;
  config.flowControl = (flow == 'H') ? 1 : 0;
  config.flowControlThreshold = 64;
  config.idleTimeout = 6;
  config.rx.maxBufSize = 128;
  config.tx.maxBufSize = 128;
  config.intEnable = 1;
  config.callBackFunc = _uartCallback;
#ifdef OS_UART_PORT
  if (HalUARTOpen(OS_UART_PORT, &config) == HAL_UART_SUCCESS)
  {
    serial[0].onread = onread;
    serial[0].onwrite = onwrite;
    return 0;
  }
#endif
 
  return 1;
}

unsigned char OS_serial_close(unsigned char port)
{
  serial[0].onread = 0;
  serial[0].onwrite = 0;
  // HalUARTClose(0); - In the hal_uart.h include file, but not actually in the code
  return 1;
}

short OS_serial_read(unsigned char port)
{
  unsigned char ch;
#ifdef OS_UART_PORT
  if (HalUARTRead(OS_UART_PORT, &ch, 1) == 1)
  {
    return ch;
  }
  else
#endif
  {
    return -1;
  }
}

unsigned char OS_serial_write(unsigned char port, unsigned char ch)
{
#ifdef OS_UART_PORT
  return HalUARTWrite(OS_UART_PORT, &ch, 1) == 1 ? 1 : 0;
#else
  return 0;
#endif
}

unsigned char OS_serial_available(unsigned char port, unsigned char ch)
{
#ifdef OS_UART_PORT
  return ch == 'R' ? Hal_UART_RxBufLen(OS_UART_PORT) : Hal_UART_TxBufLen(OS_UART_PORT);
#else
  return 0;
#endif
}

//
// Servo Motor Driver (Timer 1 Hardware 50Hz PWM & Background Ramping)
//
servo_state_t servos[MAX_SERVOS];
static volatile unsigned char servo_seq = 0;

static void servo_pin_set(unsigned char pin, unsigned char val)
{
  unsigned char minor = PIN_MINOR(pin);
  switch (PIN_MAJOR(pin))
  {
    case 0:
      if (val) P0 |= (1 << minor); else P0 &= ~(1 << minor);
      break;
    case 1:
      if (val) P1 |= (1 << minor); else P1 &= ~(1 << minor);
      break;
    case 2:
      if (val) P2 |= (1 << minor); else P2 &= ~(1 << minor);
      break;
  }
}

HAL_ISR_FUNCTION(timer1Isr, T1_VECTOR)
{
  HAL_ENTER_ISR();

  unsigned char stat = T1STAT;
  T1STAT = 0;

  if (stat & 0x21) // Timer 1 modulo compare / overflow (end of 20ms period, tick 0)
  {
    // Update ramping for active servos
    for (unsigned char s = 0; s < MAX_SERVOS; s++)
    {
      if (servos[s].active && servos[s].current_us != servos[s].target_us)
      {
        if (servos[s].step_us_x16 == 0)
        {
          servos[s].current_us = servos[s].target_us;
        }
        else
        {
          servos[s].sub_us += servos[s].step_us_x16;
          unsigned short step = servos[s].sub_us >> 4;
          if (step > 0)
          {
            servos[s].sub_us &= 0x0F;
            if (servos[s].current_us < servos[s].target_us)
            {
              servos[s].current_us += step;
              if (servos[s].current_us > servos[s].target_us)
                servos[s].current_us = servos[s].target_us;
            }
            else
            {
              if (servos[s].current_us > step)
                servos[s].current_us -= step;
              else
                servos[s].current_us = 0;
              if (servos[s].current_us < servos[s].target_us)
                servos[s].current_us = servos[s].target_us;
            }
          }
        }
      }
    }

    servo_seq = 0;
    if (servos[0].active)
    {
      servo_pin_set(servos[0].pin, 1);
      T1CC1L = (unsigned char)(servos[0].current_us & 0xFF);
      T1CC1H = (unsigned char)((servos[0].current_us >> 8) & 0xFF);
      T1CCTL1 = 0x44;
      servo_seq = 1;
    }
    else if (servos[1].active)
    {
      servo_pin_set(servos[1].pin, 1);
      T1CC1L = (unsigned char)(servos[1].current_us & 0xFF);
      T1CC1H = (unsigned char)((servos[1].current_us >> 8) & 0xFF);
      T1CCTL1 = 0x44;
      servo_seq = 3;
    }
  }

  if (stat & 0x02) // Channel 1 compare match
  {
    if (servo_seq == 1) // Servo 0 pulse end
    {
      servo_pin_set(servos[0].pin, 0);
      if (servos[1].active)
      {
        T1CC1L = (unsigned char)(3000 & 0xFF);
        T1CC1H = (unsigned char)((3000 >> 8) & 0xFF);
        servo_seq = 2;
      }
      else
      {
        T1CCTL1 = 0;
        servo_seq = 0;
      }
    }
    else if (servo_seq == 2) // Start Servo 1 pulse at tick 3000
    {
      servo_pin_set(servos[1].pin, 1);
      unsigned short pulse_end = 3000 + servos[1].current_us;
      T1CC1L = (unsigned char)(pulse_end & 0xFF);
      T1CC1H = (unsigned char)((pulse_end >> 8) & 0xFF);
      servo_seq = 3;
    }
    else if (servo_seq == 3) // Servo 1 pulse end
    {
      servo_pin_set(servos[1].pin, 0);
      T1CCTL1 = 0;
      servo_seq = 0;
    }
  }

  HAL_EXIT_ISR();
}

void OS_servo_init(void)
{
  for (unsigned char i = 0; i < MAX_SERVOS; i++)
  {
    servos[i].pin = 0;
    servos[i].active = 0;
    servos[i].current_us = 1500;
    servos[i].target_us = 1500;
    servos[i].step_us_x16 = 0;
    servos[i].sub_us = 0;
  }
  servo_seq = 0;
}

char OS_servo_attach(unsigned char pin, short initial_angle)
{
  unsigned char slot = 0xFF;
  for (unsigned char i = 0; i < MAX_SERVOS; i++)
  {
    if (servos[i].active && servos[i].pin == pin)
    {
      slot = i;
      break;
    }
  }
  if (slot == 0xFF)
  {
    for (unsigned char i = 0; i < MAX_SERVOS; i++)
    {
      if (!servos[i].active)
      {
        slot = i;
        break;
      }
    }
  }
  if (slot == 0xFF)
  {
    return 0;
  }

  if (initial_angle < 0) initial_angle = 0;
  if (initial_angle > 180) initial_angle = 180;
  unsigned short us = 1000 + (unsigned short)(((unsigned long)initial_angle * 1000) / 180);

  servos[slot].pin = pin;
  servos[slot].active = 1;
  servos[slot].current_us = us;
  servos[slot].target_us = us;
  servos[slot].step_us_x16 = 0;
  servos[slot].sub_us = 0;

  // Set pin as GPIO output
  unsigned char minor = PIN_MINOR(pin);
  switch (PIN_MAJOR(pin))
  {
    case 0:
      P0SEL &= ~(1 << minor);
      P0DIR |= (1 << minor);
      P0 &= ~(1 << minor);
      break;
    case 1:
      P1SEL &= ~(1 << minor);
      P1DIR |= (1 << minor);
      P1 &= ~(1 << minor);
      break;
    case 2:
      P2SEL &= ~(1 << minor);
      P2DIR |= (1 << minor);
      P2 &= ~(1 << minor);
      break;
  }

  // Setup Timer 1 if not already running
  T1CTL = 0;
  T1CC0L = (unsigned char)(20000 & 0xFF);
  T1CC0H = (unsigned char)((20000 >> 8) & 0xFF);
  T1CCTL0 = 0x44; // Compare mode, interrupt enabled (overflow at modulo)
  TIMIF |= 0x40;  // T1OVFIM = 1
  T1STAT = 0;
  T1CTL = 0x0A;   // DIV = /32 (1 MHz tick), MODE = Modulo
  IEN1 |= 0x02;   // Timer 1 interrupt enable

  return 1;
}

char OS_servo_detach(unsigned char pin)
{
  for (unsigned char i = 0; i < MAX_SERVOS; i++)
  {
    if (servos[i].active && servos[i].pin == pin)
    {
      servo_pin_set(pin, 0);
      servos[i].active = 0;
      servos[i].pin = 0;

      unsigned char any = 0;
      for (unsigned char j = 0; j < MAX_SERVOS; j++)
      {
        if (servos[j].active) any = 1;
      }
      if (!any)
      {
        T1CTL = 0;
        T1CCTL0 = 0;
        T1CCTL1 = 0;
        TIMIF &= ~0x40;
        IEN1 &= ~0x02;
      }
      return 1;
    }
  }
  return 0;
}

char OS_servo_set(unsigned char pin, short angle, short speed_dps)
{
  for (unsigned char i = 0; i < MAX_SERVOS; i++)
  {
    if (servos[i].active && servos[i].pin == pin)
    {
      if (angle < 0) angle = 0;
      if (angle > 180) angle = 180;
      unsigned short target = 1000 + (unsigned short)(((unsigned long)angle * 1000) / 180);

      servos[i].target_us = target;
      if (speed_dps <= 0)
      {
        servos[i].current_us = target;
        servos[i].step_us_x16 = 0;
      }
      else
      {
        servos[i].step_us_x16 = (unsigned short)(((unsigned long)speed_dps * 160) / 90);
        if (servos[i].step_us_x16 == 0) servos[i].step_us_x16 = 1;
        servos[i].sub_us = 0;
      }
      return 1;
    }
  }
  return 0;
}

short OS_servo_get(unsigned char pin)
{
  for (unsigned char i = 0; i < MAX_SERVOS; i++)
  {
    if (servos[i].active && servos[i].pin == pin)
    {
      if (servos[i].current_us <= 1000) return 0;
      if (servos[i].current_us >= 2000) return 180;
      return (short)(((unsigned long)(servos[i].current_us - 1000) * 180) / 1000);
    }
  }
  return 0;
}

//
// DC Motor Driver (Timer 3 & Timer 4 Hardware 2.5kHz PWM)
//
motor_state_t motors[MAX_MOTORS];
static volatile unsigned char motor_active_pwm[MAX_MOTORS] = {0, 0};

static void motor_pin_config(unsigned char pin)
{
  unsigned char minor = PIN_MINOR(pin);
  switch (PIN_MAJOR(pin))
  {
    case 0:
      P0SEL &= ~(1 << minor);
      P0DIR |= (1 << minor);
      P0 &= ~(1 << minor);
      break;
    case 1:
      P1SEL &= ~(1 << minor);
      P1DIR |= (1 << minor);
      P1 &= ~(1 << minor);
      break;
    case 2:
      P2SEL &= ~(1 << minor);
      P2DIR |= (1 << minor);
      P2 &= ~(1 << minor);
      break;
  }
}

static void motor_apply_outputs(unsigned char slot)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return;

  motor_state_t* m = &motors[slot];
  unsigned char abs_speed = (m->speed < 0) ? (unsigned char)(-m->speed) : (unsigned char)(m->speed);

  if (m->mode == 3) // BRAKE
  {
    servo_pin_set(m->pin_fwd, 1);
    servo_pin_set(m->pin_rev, 1);
    if (m->pin_pwm) servo_pin_set(m->pin_pwm, 1);
    motor_active_pwm[slot] = 0;
  }
  else if (m->mode == 0 || abs_speed == 0) // COAST / STOP
  {
    servo_pin_set(m->pin_fwd, 0);
    servo_pin_set(m->pin_rev, 0);
    if (m->pin_pwm) servo_pin_set(m->pin_pwm, 0);
    motor_active_pwm[slot] = 0;
  }
  else if (m->mode == 1) // FORWARD
  {
    if (m->pin_pwm)
    {
      servo_pin_set(m->pin_fwd, 1);
      servo_pin_set(m->pin_rev, 0);
      if (abs_speed >= 100)
      {
        servo_pin_set(m->pin_pwm, 1);
        motor_active_pwm[slot] = 0;
      }
      else
      {
        motor_active_pwm[slot] = m->pin_pwm;
      }
    }
    else
    {
      servo_pin_set(m->pin_rev, 0);
      if (abs_speed >= 100)
      {
        servo_pin_set(m->pin_fwd, 1);
        motor_active_pwm[slot] = 0;
      }
      else
      {
        motor_active_pwm[slot] = m->pin_fwd;
      }
    }
  }
  else if (m->mode == 2) // REVERSE
  {
    if (m->pin_pwm)
    {
      servo_pin_set(m->pin_fwd, 0);
      servo_pin_set(m->pin_rev, 1);
      if (abs_speed >= 100)
      {
        servo_pin_set(m->pin_pwm, 1);
        motor_active_pwm[slot] = 0;
      }
      else
      {
        motor_active_pwm[slot] = m->pin_pwm;
      }
    }
    else
    {
      servo_pin_set(m->pin_fwd, 0);
      if (abs_speed >= 100)
      {
        servo_pin_set(m->pin_rev, 1);
        motor_active_pwm[slot] = 0;
      }
      else
      {
        motor_active_pwm[slot] = m->pin_rev;
      }
    }
  }

  // Hardware PWM timers
  if (slot == 0)
  {
    if (motor_active_pwm[0] && abs_speed > 0 && abs_speed < 100)
    {
      T3CTL = 0;
      T3CC0 = 100;
      T3CC1 = abs_speed;
      T3CCTL0 = 0x44; // Compare mode, IM=1
      T3CCTL1 = 0x44; // Compare mode, IM=1
      TIMIF &= ~(0x01 | 0x02 | 0x04);
      T3CTL = 0xF2;   // /128 prescaler, START=1, Modulo mode (0..T3CC0)
      IEN1 |= 0x08;   // Enable Timer 3 interrupt (T3IE)
    }
    else
    {
      T3CTL = 0;      // Stop Timer 3
      IEN1 &= ~0x08;  // Disable Timer 3 interrupt
    }
  }
  else if (slot == 1)
  {
    if (motor_active_pwm[1] && abs_speed > 0 && abs_speed < 100)
    {
      T4CTL = 0;
      T4CC0 = 100;
      T4CC1 = abs_speed;
      T4CCTL0 = 0x44; // Compare mode, IM=1
      T4CCTL1 = 0x44; // Compare mode, IM=1
      TIMIF &= ~(0x08 | 0x10 | 0x20);
      T4CTL = 0xF2;   // /128 prescaler, START=1, Modulo mode (0..T4CC0)
      IEN1 |= 0x10;   // Enable Timer 4 interrupt (T4IE)
    }
    else
    {
      T4CTL = 0;      // Stop Timer 4
      IEN1 &= ~0x10;  // Disable Timer 4 interrupt
    }
  }
}

HAL_ISR_FUNCTION(timer3Isr, T3_VECTOR)
{
  HAL_ENTER_ISR();
  unsigned char timif = TIMIF;
  TIMIF &= ~(0x01 | 0x02 | 0x04);
  T3IF = 0;

  if (timif & (0x01 | 0x02)) // Modulo match / period start
  {
    if (motor_active_pwm[0])
    {
      servo_pin_set(motor_active_pwm[0], 1);
    }
  }
  if (timif & 0x04) // Duty cycle compare match (T3CC1)
  {
    if (motor_active_pwm[0])
    {
      servo_pin_set(motor_active_pwm[0], 0);
    }
  }

  HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(timer4Isr, T4_VECTOR)
{
  HAL_ENTER_ISR();
  unsigned char timif = TIMIF;
  TIMIF &= ~(0x08 | 0x10 | 0x20);
  T4IF = 0;

  if (timif & (0x08 | 0x10)) // Modulo match / period start
  {
    if (motor_active_pwm[1])
    {
      servo_pin_set(motor_active_pwm[1], 1);
    }
  }
  if (timif & 0x20) // Duty cycle compare match (T4CC1)
  {
    if (motor_active_pwm[1])
    {
      servo_pin_set(motor_active_pwm[1], 0);
    }
  }

  HAL_EXIT_ISR();
}

void OS_motor_init(void)
{
  for (unsigned char i = 0; i < MAX_MOTORS; i++)
  {
    motors[i].pin_fwd = 0;
    motors[i].pin_rev = 0;
    motors[i].pin_pwm = 0;
    motors[i].attached = 0;
    motors[i].speed = 0;
    motors[i].mode = 0;
    motor_active_pwm[i] = 0;
  }
  T3CTL = 0;
  T4CTL = 0;
  IEN1 &= ~(0x08 | 0x10);
}

char OS_motor_attach(unsigned char slot, unsigned char pin_fwd, unsigned char pin_rev, unsigned char pin_pwm)
{
  if (slot >= MAX_MOTORS) return 0;

  motors[slot].pin_fwd = pin_fwd;
  motors[slot].pin_rev = pin_rev;
  motors[slot].pin_pwm = pin_pwm;
  motors[slot].attached = 1;
  motors[slot].speed = 0;
  motors[slot].mode = 0;

  motor_pin_config(pin_fwd);
  motor_pin_config(pin_rev);
  if (pin_pwm) motor_pin_config(pin_pwm);

  motor_apply_outputs(slot);
  return 1;
}

char OS_motor_detach(unsigned char slot)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;

  motors[slot].speed = 0;
  motors[slot].mode = 0;
  motor_apply_outputs(slot);

  motors[slot].attached = 0;
  motors[slot].pin_fwd = 0;
  motors[slot].pin_rev = 0;
  motors[slot].pin_pwm = 0;
  return 1;
}

char OS_motor_forward(unsigned char slot, unsigned char speed)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  if (speed > 100) speed = 100;
  motors[slot].mode = 1;
  motors[slot].speed = (signed char)speed;
  motor_apply_outputs(slot);
  return 1;
}

char OS_motor_reverse(unsigned char slot, unsigned char speed)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  if (speed > 100) speed = 100;
  motors[slot].mode = 2;
  motors[slot].speed = -(signed char)speed;
  motor_apply_outputs(slot);
  return 1;
}

char OS_motor_brake(unsigned char slot)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  motors[slot].mode = 3;
  motors[slot].speed = 0;
  motor_apply_outputs(slot);
  return 1;
}

char OS_motor_coast(unsigned char slot)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  motors[slot].mode = 0;
  motors[slot].speed = 0;
  motor_apply_outputs(slot);
  return 1;
}

char OS_motor_set_speed(unsigned char slot, unsigned char speed)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  if (speed > 100) speed = 100;
  if (motors[slot].mode == 2)
  {
    motors[slot].speed = -(signed char)speed;
  }
  else
  {
    motors[slot].mode = 1;
    motors[slot].speed = (signed char)speed;
  }
  motor_apply_outputs(slot);
  return 1;
}

signed short OS_motor_get(unsigned char slot)
{
  if (slot >= MAX_MOTORS || !motors[slot].attached) return 0;
  return (signed short)motors[slot].speed;
}
