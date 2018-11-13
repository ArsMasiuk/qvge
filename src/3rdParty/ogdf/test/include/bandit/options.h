#ifndef BANDIT_OPTIONS_H
#define BANDIT_OPTIONS_H

#include <algorithm>
#include <vector>
#include <iostream>

#include <bandit/external/optionparser.h>
#include <bandit/filter_chain.h>

namespace bandit {
  namespace detail {
    struct options {
      template<typename ENUM>
      struct argstr {
        ENUM id;
        std::string str;
      };

      // a vector of argstr that allows to iterate over the strings only
      template<typename ENUM>
      struct argstrs : std::vector<argstr<ENUM>> {
        using std::vector<argstr<ENUM>>::vector;

        struct str_iterator
            : public std::iterator<std::input_iterator_tag, std::string, int, const std::string*, std::string> {
          using base_iterator = typename std::vector<argstr<ENUM>>::const_iterator;

          str_iterator() = delete;

          explicit str_iterator(base_iterator it) : it_(it) {}

          str_iterator& operator++() {
            ++it_;
            return *this;
          }

          str_iterator operator++(int) {
            str_iterator it(*this);
            ++(*this);
            return it;
          }

          bool operator==(const str_iterator& other) const {
            return it_ == other.it_;
          }

          bool operator!=(const str_iterator& other) const {
            return it_ != other.it_;
          }

          reference operator*() const {
            return it_->str;
          }

          ENUM id() {
            return it_->id;
          }

        private:
          base_iterator it_;
        };

        str_iterator strbegin() const {
          return str_iterator(this->begin());
        };

        str_iterator strend() const {
          return str_iterator(this->end());
        };
      };

      enum class formatters {
        DEFAULT,
        VS,
        UNKNOWN
      };

      enum class reporters {
        SINGLELINE,
        XUNIT,
        INFO,
        SPEC,
        DOTS,
        CRASH,
        UNKNOWN
      };

      struct argument : public option::Arg {
        static const argstrs<reporters> reporter_list() {
          return {
              {reporters::CRASH, "crash"},
              {reporters::DOTS, "dots"},
              {reporters::SINGLELINE, "singleline"},
              {reporters::XUNIT, "xunit"},
              {reporters::INFO, "info"},
              {reporters::SPEC, "spec"},
          };
        }

        static const argstrs<formatters> formatter_list() {
          return {
              {formatters::DEFAULT, "default"},
              {formatters::VS, "vs"},
          };
        }

        template<typename ENUM>
        static std::string comma_separated_list(argstrs<ENUM> list) {
          std::string csl;
          auto first = list.strbegin();
          if (first != list.strend()) {
            csl += *first;
            std::for_each(++first, list.strend(), [&](const std::string& elem) {
              csl += ", " + elem;
            });
          }
          return csl;
        }

        static std::string name(const option::Option& option) {
          std::string copy(option.name);
          return copy.substr(0, option.namelen);
        }

        static option::ArgStatus Required(const option::Option& option, bool msg) {
          if (option.arg != nullptr) {
            return option::ARG_OK;
          }
          if (msg) {
            std::cerr << "Option '" << name(option) << "' requires an argument\n";
          }
          return option::ARG_ILLEGAL;
        }

        template<typename ENUM>
        static option::ArgStatus OneOf(const option::Option& option, bool msg, const argstrs<ENUM>&& list) {
          auto status = Required(option, msg);
          if (status == option::ARG_OK && std::find(list.strbegin(), list.strend(), option.arg) == list.strend()) {
            if (msg) {
              std::cerr
                  << "Option argument of '" << name(option) << "' must be one of: "
                  << comma_separated_list(list)
                  << std::endl;
            }
            status = option::ARG_ILLEGAL;
          }
          return status;
        }

        static option::ArgStatus Reporter(const option::Option& option, bool msg) {
          return OneOf(option, msg, reporter_list());
        }

        static option::ArgStatus Formatter(const option::Option& option, bool msg) {
          return OneOf(option, msg, formatter_list());
        }
      };

      options(int argc, char* argv[]) {
        argc -= (argc > 0);
        argv += (argc > 0); // Skip program name (argv[0]) if present
        option::Stats stats(usage(), argc, argv);
        options_.resize(stats.options_max);
        std::vector<option::Option> buffer(stats.buffer_max);
        option::Parser parse(usage(), argc, argv, options_.data(), buffer.data());
        parsed_ok_ = !parse.error();
        has_further_arguments_ = (parse.nonOptionsCount() != 0);
        has_unknown_options_ = (options_[UNKNOWN] != nullptr);

        for (int i = 0; i < parse.optionsCount(); ++i) {
          option::Option& opt = buffer[i];
          switch (opt.index()) {
          case SKIP:
            filter_chain_.push_back({opt.arg, true});
            break;
          case ONLY:
            filter_chain_.push_back({opt.arg, false});
            break;
          }
        }
      }

      bool help() const {
        return options_[HELP] != nullptr;
      }

      bool parsed_ok() const {
        return parsed_ok_;
      }

      bool has_further_arguments() const {
        return has_further_arguments_;
      }

      bool has_unknown_options() const {
        return has_unknown_options_;
      }

      void print_usage() const {
        option::printUsage(std::cout, usage());
      }

      bool version() const {
        return options_[VERSION] != nullptr;
      }

      reporters reporter() const {
        return get_enumerator_from_string(argument::reporter_list(), options_[REPORTER].arg);
      }

      bool no_color() const {
        return options_[NO_COLOR] != nullptr;
      }

      formatters formatter() const {
        return get_enumerator_from_string(argument::formatter_list(), options_[FORMATTER].arg);
      }

      const filter_chain_t& filter_chain() const {
        return filter_chain_;
      }

      bool break_on_failure() const {
        return options_[BREAK_ON_FAILURE] != nullptr;
      }

      bool dry_run() const {
        return options_[DRY_RUN] != nullptr;
      }

    private:
      template<typename ENUM>
      ENUM get_enumerator_from_string(const argstrs<ENUM>& list, const char* str) const {
        if (str != nullptr) {
          auto it = std::find(list.strbegin(), list.strend(), str);
          if (it != list.strend()) {
            return it.id();
          }
        }
        return ENUM::UNKNOWN;
      }

      enum option_index {
        UNKNOWN,
        VERSION,
        HELP,
        REPORTER,
        NO_COLOR,
        FORMATTER,
        SKIP,
        ONLY,
        BREAK_ON_FAILURE,
        DRY_RUN,
      };

      template<typename ENUM>
      static std::string append_list(std::string desc, argstrs<ENUM> list) {
        return desc + ": " + argument::comma_separated_list(list);
      }

      static const option::Descriptor* usage() {
        static std::string reporter_help = append_list("  --reporter=<reporter>, "
            "\tSelect reporter", argument::reporter_list());
        static std::string formatter_help = append_list("  --formatter=<formatter>, "
            "\tSelect error formatter", argument::formatter_list());
        static const option::Descriptor usage[] = {
            {UNKNOWN, 0, "", "", argument::None,
                "USAGE: <executable> [options]\n\n"
                "Options:"},
            {VERSION, 0, "", "version", argument::None,
                "  --version, \tPrint version of bandit"},
            {HELP, 0, "", "help", argument::None,
                "  --help, \tPrint usage and exit."},
            {REPORTER, 0, "", "reporter", argument::Reporter, reporter_help.c_str()},
            {NO_COLOR, 0, "", "no-color", argument::None,
                "  --no-color, \tSuppress colors in output"},
            {FORMATTER, 0, "", "formatter", argument::Formatter, formatter_help.c_str()},
            {SKIP, 0, "", "skip", argument::Required,
                "  --skip=<substring>, \tSkip all 'describe' and 'it' containing substring"},
            {ONLY, 0, "", "only", argument::Required,
                "  --only=<substring>, \tRun only 'describe' and 'it' containing substring"},
            {BREAK_ON_FAILURE, 0, "", "break-on-failure", argument::None,
                "  --break-on-failure, \tStop test run on first failing test"},
            {DRY_RUN, 0, "", "dry-run", argument::None,
                "  --dry-run, \tSkip all tests. Use to list available tests"},
            {0, 0, 0, 0, 0, 0}};

        return usage;
      }

    private:
      std::vector<option::Option> options_;
      filter_chain_t filter_chain_;
      bool parsed_ok_;
      bool has_further_arguments_;
      bool has_unknown_options_;
    };
  }
}
#endif
