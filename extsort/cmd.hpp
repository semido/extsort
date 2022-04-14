#pragma once

#include <string>
#include <vector>

// An Option could be parametric, something like '-n 5',
//   or boolean, this must start with 2 minuses, like '--diff'.
// Also positional arguments could be passed, those should not have
//   leading '-'. Method 'get_params' collects it to a container.
// Unicode names supported in std::string utf-8 format.
class CmdOptions
{
public:

  CmdOptions(int argc, const char* argv[]);

  std::string get_option(const std::string& option, const std::string& def = "") const; // Find parameter of the parametric option, u.g. '-n 5'.
  bool exists_option(const std::string& option) const; // Check if the option is provided.
  void get_params(std::vector<std::string>& params) const; // Collect all positional arguments, i.e. not a part of parametric option.

  bool empty() const { return m_argv.size() <= 1; } // I.e. no args.
  size_t size() const { return m_argv.size(); } // NB: [0] is program name.
  const std::string& operator[](size_t i) const { return m_argv.at(i); }

private:

  std::vector<std::string> m_argv;
};
