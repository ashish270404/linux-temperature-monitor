// temp_monitor.cpp
// Opens /dev/tempsensor fresh each second, drives a small state machine
// (NORMAL -> WARNING -> CRITICAL), and logs a message on every transition.
//
// Usage:
//   sudo ./temp_monitor                 run the monitoring loop
//   sudo ./temp_monitor --reset         send TEMP_IOC_RESET, then exit
//   sudo ./temp_monitor --drift N       send TEMP_IOC_SET_DRIFT with value N
//                                       (tenths of a degree), then run the loop
//
// Capstone teaching app - not for production use.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../driver/tempsensor_ioctl.h"

static const char *DEV_PATH = "/dev/tempsensor";

enum class State { NORMAL, WARNING, CRITICAL };

static const char *state_name(State s)
{
    switch (s) {
        case State::NORMAL:   return "NORMAL";
        case State::WARNING:  return "WARNING";
        case State::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

static State classify(double temp_c)
{
    if (temp_c > 80.0) return State::CRITICAL;
    if (temp_c >= 60.0) return State::WARNING;
    return State::NORMAL;
}

static std::string timestamp()
{
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

// Opens the device, does exactly one read, closes it. Avoids relying on
// lseek()/persistent file position, which plain character devices like
// this one don't support.
static double read_temperature()
{
    int fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1.0;
    }

    char buf[16] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0) {
        fprintf(stderr, "read returned %zd\n", n);
        return -1.0;
    }
    buf[n] = '\0';
    return std::atof(buf);
}

int main(int argc, char *argv[])
{
    // --reset : one-shot ioctl, then exit
    if (argc == 2 && std::strcmp(argv[1], "--reset") == 0) {
        int fd = open(DEV_PATH, O_RDONLY);
        if (fd < 0) {
            perror("open (is the module loaded? are you root?)");
            return 1;
        }
        if (ioctl(fd, TEMP_IOC_RESET) < 0) {
            perror("ioctl RESET");
            close(fd);
            return 1;
        }
        printf("[%s] Sensor reset to baseline.\n", timestamp().c_str());
        close(fd);
        return 0;
    }

    // --drift N : set drift, then continue into the monitoring loop
    if (argc == 3 && std::strcmp(argv[1], "--drift") == 0) {
        int fd = open(DEV_PATH, O_RDONLY);
        if (fd < 0) {
            perror("open (is the module loaded? are you root?)");
            return 1;
        }
        int drift = std::atoi(argv[2]);
        if (ioctl(fd, TEMP_IOC_SET_DRIFT, &drift) < 0) {
            perror("ioctl SET_DRIFT");
            close(fd);
            return 1;
        }
        printf("[%s] Drift set to %.1f C per reading.\n",
               timestamp().c_str(), drift / 10.0);
        close(fd);
    }

    // sanity check the device exists / is openable before starting the loop
    {
        int fd = open(DEV_PATH, O_RDONLY);
        if (fd < 0) {
            perror("open (is the module loaded? are you root?)");
            return 1;
        }
        close(fd);
    }

    printf("[%s] Monitoring started. Press Ctrl+C to stop.\n",
           timestamp().c_str());

    State current_state = State::NORMAL;
    printf("[%s] Initial state: %s\n", timestamp().c_str(), state_name(current_state));

    while (true) {
        double temp = read_temperature();
        if (temp < 0) {
            fprintf(stderr, "[%s] Failed to read sensor, retrying...\n",
                    timestamp().c_str());
        } else {
            State new_state = classify(temp);

            if (new_state != current_state) {
                printf("[%s] TRANSITION: %s -> %s  (temp = %.1f C)\n",
                       timestamp().c_str(),
                       state_name(current_state), state_name(new_state),
                       temp);
                current_state = new_state;
            } else {
                printf("[%s] temp = %.1f C  (state: %s)\n",
                       timestamp().c_str(), temp, state_name(current_state));
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
