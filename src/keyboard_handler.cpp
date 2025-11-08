// keyboard_handler.cpp - Keyboard handler implementation

#include "keyboard_handler.h"

#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace jettison
{

KeyboardHandler::KeyboardHandler (CommandQueue &queue,
                                   std::atomic<bool> &running)
    : queue_ (queue), running_ (running)
{
  set_nonblocking_mode ();
}

KeyboardHandler::~KeyboardHandler ()
{
  restore_terminal_mode ();
}

void
KeyboardHandler::set_nonblocking_mode ()
{
  // Save old terminal settings
  struct termios *oldt = new struct termios;
  tcgetattr (STDIN_FILENO, oldt);
  old_termios_ = oldt;

  // Set terminal to non-canonical mode (no line buffering)
  struct termios newt = *oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr (STDIN_FILENO, TCSANOW, &newt);

  // Set stdin to non-blocking
  int flags = fcntl (STDIN_FILENO, F_GETFL, 0);
  fcntl (STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void
KeyboardHandler::restore_terminal_mode ()
{
  if (old_termios_)
    {
      auto *oldt = static_cast<struct termios *> (old_termios_);
      tcsetattr (STDIN_FILENO, TCSANOW, oldt);
      delete oldt;
      old_termios_ = nullptr;
    }

  // Restore blocking mode
  int flags = fcntl (STDIN_FILENO, F_GETFL, 0);
  fcntl (STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

void
KeyboardHandler::check_input ()
{
  char c;
  ssize_t n = read (STDIN_FILENO, &c, 1);

  if (n == 1)
    {
      handle_key (c);
    }
  // n == 0 or n == -1 with EAGAIN means no input available (normal)
}

void
KeyboardHandler::handle_key (char key)
{
  // Optical zoom: 1-5 � indices 0-4
  if (key >= '1' && key <= '5')
    {
      int index = key - '1';
      Command cmd = Command::optical_zoom (index);
      queue_.push (cmd);

#ifdef CAMERA_TYPE_HEAT
      char label = 'A' + index;
#else
      char label = 'I' + index;
#endif

      std::cout << "\n� Queued optical zoom to position " << index << " ('"
                << label << "')\n";
    }
  // Digital zoom: a-k � 1.0x - 6.0x (0.5x steps)
  else if (key >= 'a' && key <= 'k')
    {
      int step = key - 'a';
      float level = 1.0f + (step * 0.5f);
      Command cmd = Command::digital_zoom (level);
      queue_.push (cmd);

      std::cout << "\n� Queued digital zoom to " << level << "x\n";
    }
  // Invalid zoom test: 'z' sends out-of-range zoom index
  // This tests buf/validate validation on the client
  else if (key == 'z' || key == 'Z')
    {
      // Send invalid zoom index (-1 violates gte: 0 constraint)
      Command cmd = Command::optical_zoom (-1);
      queue_.push (cmd);

      std::cout << "\n→ Queued INVALID optical zoom (index: -1)\n";
      std::cout << "  Testing buf.validate - command should be rejected "
                   "locally\n";
    }
  // Quit
  else if (key == 'q' || key == 'Q')
    {
      std::cout << "\n� Quit requested\n";
      running_.store (false);
    }
  // Help
  else if (key == 'h' || key == 'H' || key == '?')
    {
      print_help ();
    }
  else
    {
      // Unknown key - ignore silently
    }
}

void
KeyboardHandler::print_help ()
{
  std::cout << "\n";
  std::cout << "\n";
  std::cout << "Keyboard Controls\n";
  std::cout << "\n";
  std::cout << "\n";
  std::cout << "Optical Zoom:\n";
  std::cout << "  1-5     Set zoom table position (";
#ifdef CAMERA_TYPE_HEAT
  std::cout << "A-E";
#else
  std::cout << "I-V";
#endif
  std::cout << ")\n";
  std::cout << "\n";
  std::cout << "Digital Zoom:\n";
  std::cout << "  a-k     Set digital zoom level\n";
  std::cout << "          a=1.0x, b=1.5x, c=2.0x, d=2.5x, e=3.0x\n";
  std::cout << "          f=3.5x, g=4.0x, h=4.5x, i=5.0x, j=5.5x, k=6.0x\n";
  std::cout << "\n";
  std::cout << "Testing:\n";
  std::cout << "  z/Z     Send INVALID zoom (tests server validation)\n";
  std::cout << "\n";
  std::cout << "Other:\n";
  std::cout << "  q/Q     Quit\n";
  std::cout << "\n";
  std::cout << "\n";
  std::cout << "\n";
}

} // namespace jettison
