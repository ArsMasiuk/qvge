#ifndef BANDIT_REPORTERS_COLORIZER_H
#define BANDIT_REPORTERS_COLORIZER_H

#include <string>

#if defined(_WIN32) && !defined(BANDIT_CONFIG_COLOR_ANSI)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace bandit {
  namespace detail {
#if defined(_WIN32) && !defined(BANDIT_CONFIG_COLOR_ANSI)
    struct colorizer {
      colorizer(bool colors_enabled = true)
          : colors_enabled_(colors_enabled),
            stdout_handle_(GetStdHandle(STD_OUTPUT_HANDLE)) {
        background_color_ = original_color_ = get_console_color();
        background_color_ &= BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
      }

      const std::string green() const {
        if (colors_enabled_) {
          set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY | background_color_);
        }
        return "";
      }

      const std::string yellow() const {
        if (colors_enabled_) {
          set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | background_color_);
        }
        return "";
      }

      const std::string blue() const {
        if (colors_enabled_) {
          set_console_color(FOREGROUND_BLUE | FOREGROUND_INTENSITY | background_color_);
        }
        return "";
      }

      const std::string red() const {
        if (colors_enabled_) {
          set_console_color(FOREGROUND_RED | FOREGROUND_INTENSITY | background_color_);
        }
        return "";
      }

      const std::string white() const {
        if (colors_enabled_) {
          set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
              FOREGROUND_INTENSITY | background_color_);
        }
        return "";
      }

      const std::string reset() const {
        if (colors_enabled_) {
          set_console_color(original_color_);
        }
        return "";
      }

    private:
      WORD get_console_color() const {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        GetConsoleScreenBufferInfo(stdout_handle_, &info);
        return info.wAttributes;
      }

      void set_console_color(WORD color) const {
        SetConsoleTextAttribute(stdout_handle_, color);
      }

    private:
      bool colors_enabled_;
      HANDLE stdout_handle_;
      WORD original_color_;
      WORD background_color_;
    };
#else
    struct colorizer {
      colorizer(bool colors_enabled = true) : colors_enabled_(colors_enabled) {}

      const std::string green() const {
        return colors_enabled_ ? "\033[1;32m" : "";
      }

      const std::string yellow() const {
        return colors_enabled_ ? "\033[1;33m" : "";
      }

      const std::string blue() const {
        return colors_enabled_ ? "\033[1;34m" : "";
      }

      const std::string red() const {
        return colors_enabled_ ? "\033[1;31m" : "";
      }

      const std::string white() const {
        return colors_enabled_ ? "\033[1;37m" : "";
      }

      const std::string reset() const {
        return colors_enabled_ ? "\033[0m" : "";
      }

    private:
      bool colors_enabled_;
    };
#endif
  }
}
#endif
