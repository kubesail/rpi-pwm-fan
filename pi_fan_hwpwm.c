/*
/
/ pi_fan_pwm.c, alwynallan@gmail.com 12/2020, no license
/
/ Need    http://www.airspayce.com/mikem/bcm2835/index.html
/
/ Compile $ gcc -Wall pi_fan_pwm.c -lpigpio -lpthread -o pi_fan_pwm
/
/ Disable $ sudo nano /boot/config.txt            [Raspbian, or use GUI]
          $ sudo nano /boot/firmware/usercfg.txt  [Ubuntu]
/             # dtoverlay=gpio-fan,gpiopin=14,temp=80000 <-------------- commented out, reboot
/
/ Run     $ sudo ./pi_fan_pwm -v
/
/ Forget  $ sudo ./pi_fan_pwm &
/         $ disown -a
/
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdarg.h>
#include <bcm2835.h>

#define PWM_PIN   13
#define HIGH_TEMP 80.
#define ON_TEMP   65.
#define OFF_TEMP  60.
#define MIN_FAN   150
#define KICK_FAN  200
#define MAX_FAN   480

unsigned pin = PWM_PIN;
int verbose = 0;
int fan_state = 0;
double temp = 25.0;
pid_t global_pid;

void usage()
{
   fprintf
   (stderr,
      "\n" \
      "Usage: sudo ./pi_fan_hwpwm [OPTION]...\n" \
      "\n" \
      "  -g <n> Use Broadcom GPIO n for fan's PWM input, default %d\n" \
      "         Only GPIO 18 and GPIO 13 are present on the RasPi 4B pin header,\n" \
      "         and only GPIO 13 is known to work. 
      "  -v     Verbose output\n" \
      "\n"
      , PWM_PIN
   );
}

void fatal(int show_usage, char *fmt, ...) {
   char buf[128];
   va_list ap;

   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);
   fprintf(stderr, "%s\n", buf);
   if (show_usage) usage();
   fflush(stderr);
   exit(EXIT_FAILURE);
}

void run_write(const char *fname, const char *data) {
// https://opensource.com/article/19/4/interprocess-communication-linux-storage
  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = global_pid;
  int fd;
  if ((fd = open(fname, O_RDWR | O_CREAT, 0666)) < 0)
    fatal(0, "failed to open %s for writing", fname);
  if (fcntl(fd, F_SETLK, &lock) < 0)
    fatal(0, "fcntl failed to get lock on %s", fname);
  if (ftruncate(fd, 0) < 0)
    fatal(0, "truncate failed to on %s", fname);
  write(fd, data, strlen(data));
  close(fd);
}

void fan_loop(void) {
  if(!fan_state && (temp > ON_TEMP)) {
    gpioPWM(pin, KICK_FAN);
    fan_state = 1;
    return;
  }
  if(fan_state && (temp < OFF_TEMP)) {
    gpioPWM(pin, 0);
    fan_state = 0;
    return;
  }
  if(fan_state) {
    unsigned out = (double) MIN_FAN + (temp - OFF_TEMP) / (HIGH_TEMP - OFF_TEMP) * (double)(MAX_FAN - MIN_FAN);
    if(out > 1000) out = 1000;
    gpioPWM(pin, out);
  }
}

int main(int argc, char *argv[]) {
  int opt;
  unsigned loop = 0;
  int t;
  FILE *ft;
  char buf[100];

  while ((opt = getopt(argc, argv, "g:v")) != -1) {
    switch (opt) {
    case 'g':
      pin = atoi(optarg);
      if(pin > 31) fatal(0, "Invalid GPIO");
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      usage();
      exit(EXIT_FAILURE);
    }
  }
  if(optind != argc) fatal(1, "optind=%d argc=%d Unrecognized parameter %s", optind, argc, argv[optind]);

  global_pid = getpid();
  sprintf(buf, "%d\n", global_pid);
  run_write("/run/pi_fan_pwm.pid", buf);

  gpioCfgClock(2, 1, 1);
  if(gpioInitialise()<0) fatal(0, "gpioInitialise() failed");
  if(gpioSetPWMfrequency(pin, 20000) != 20000) fatal(0, "PWM freqency error");
  gpioSetPWMrange(pin, 1000);
  gpioPWM(pin, 0);

  while(1) {
    loop++;
    ft = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    fscanf(ft, "%d", &t);
    fclose(ft);
    temp = 0.0001 * (double)t + 0.9 * temp;
    if((loop%4) == 0) { // every second
      fan_loop();
      sprintf(buf, "%u, %.2f, %d\n", loop/4, temp, gpioGetPWMdutycycle(pin));
      run_write("/run/pi_fan_pwm.state", buf);
      if(verbose) fputs(buf, stdout);
    }
    usleep(250000);
  }

  gpioTerminate();
  exit(EXIT_SUCCESS);
}
