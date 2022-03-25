#define __cpp
#include "ak_extern.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define FIFOPATH "/tmp/ptz.daemon"

std::vector<std::string> operations = std::vector<std::string>();
int shuttingDown = 0;
int commandThreadDone = 0;

/* stolen from
 * https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
 */
std::vector<std::string> splitString(const std::string &str, char delim) {
  std::vector<std::string> tokens;
  if (str == "")
    return tokens;
  std::string currentToken;
  std::stringstream ss(str);
  while (std::getline(ss, currentToken, delim)) {
    tokens.push_back(currentToken);
  }
  return tokens;
}

void process_command(const std::string l) {
  std::cout << "spawned processing command thread."
            << "\n";

  std::vector<std::string> cmd = splitString(l, ' ');
  std::cout << "parameters: \n";
  for (auto i : cmd) {
    std::cout << i << "\n";
  }

  if (cmd[0] == "init") {
    std::cout << "init ptz driver (anyka)"
              << "\n";
    ak_drv_ptz_open();
    ak_drv_ptz_check_self(0);
    ak_drv_ptz_set_degree(0x168, 0xc0);
    ak_drv_ptz_set_angle_rate(0x40000000, 0x40000000);
  } else if (cmd[0] == "setar") {
    std::cout << "set angle rate"
              << "\n";
    ak_drv_ptz_set_angle_rate(atoll(cmd[1].c_str()), atoll(cmd[2].c_str()));
  } else if (cmd[0] == "setdeg") {
    std::cout << "set degree"
              << "\n";
    ak_drv_ptz_set_degree(atoll(cmd[1].c_str()), atoll(cmd[2].c_str()));

  } else if (cmd[0] == "t2p") {
    std::cout << "turn to position"
              << "\n";
    ak_drv_ptz_turn_to_pos(atoll(cmd[1].c_str()), atoll(cmd[2].c_str()));
  } else if (cmd[0] == "setspeed") {
    std::cout << "set speed (motor, speed)"
              << "\n";
    ak_drv_ptz_set_speed(atoll(cmd[1].c_str()), atoll(cmd[2].c_str()),0,0);

  } else if (cmd[0] == "t") {
    std::cout << "turn"
              << "\n";
    ak_drv_ptz_turn(atoll(cmd[1].c_str()), atoll(cmd[2].c_str()), 0);

  } else if (cmd[0] == "q") {
    shuttingDown = 1;
  } else if (cmd[0] == "ping") {
    std::cout << "pong"
              << "\n";
  }
  commandThreadDone = 1;
}

void thread_ready() {
  std::cout << "This thread is ready to be torn down!\n";
  commandThreadDone = 1;
}

void handle_fifo() {

  std::ifstream fifo;

  while (shuttingDown == 0) {
    fifo.open(FIFOPATH, std::ifstream::in);
    std::string l;
    while (std::getline(fifo, l)) {
      if (l.length() > 0)
        std::cout << "received something in the FIFO:" << l << "\n";
      else
        continue;

      std::cout << "Added operation to buffer\n";
      operations.push_back(l);
    }
    fifo.close();
    sleep(1);
  }
}

int main(int argc, char **argv) {

  struct stat buf;
  if (stat(FIFOPATH, &buf) != 0) {
    std::cout << "creating FIFO in /tmp..."
              << "\n";

    if ((mkfifo(FIFOPATH, 0700)) != 0) {
      std::cerr << "could not create fifo @ " << FIFOPATH << "\n";
      return 1;
    }
  }
  std::cout << "init ptz driver (anyka)\n";
  std::cout << "running as daemon now. awaiting commands in FIFO\n";

  shuttingDown = 0;
  std::thread t = std::thread(thread_ready);
  std::thread handleFIFO = std::thread(handle_fifo);
  while (shuttingDown == 0) {
    if (operations.size() > 0 && (!t.joinable() || commandThreadDone == 1)) {
      commandThreadDone = 0;
      std::cout << "thread is done - joining thread...\n";
      if (t.joinable())
        t.join();
      if (operations.size() > 0) {
        std::string line = operations.front();
        operations.erase(operations.begin());
        t = std::thread(process_command, line.c_str());
      }
    } else if (operations.size() > 0) {
      std::cout << "previous job is still running...\n";
    } else {
    }
    std::cout << "there are currently " << operations.size()
              << " operations in queue.\n";

    sleep(1);
  }
  return 0;
}
