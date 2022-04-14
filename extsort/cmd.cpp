#include "cmd.hpp"
#include <algorithm>

CmdOptions::CmdOptions(int argc, const char* argv[])
{
  m_argv.reserve(argc);
  for (size_t i = 0; i < argc; i++)
    m_argv.emplace_back(argv[i]);
}

std::string CmdOptions::get_option(const std::string& option, const std::string& def) const
{
  if (option.size() < 2 || option[0] != '-' || option[1] == '-')
    return def;
  const auto st = m_argv.begin() + 1;
  const auto en = m_argv.end();
  auto itr = std::find(st, en, option);
  if (itr != en && ++itr != en)
    return *itr;
  return def;
}

bool CmdOptions::exists_option(const std::string& option) const
{
  if (option.size() < 2 || option[0] != '-')
    return false;
  const auto st = m_argv.begin() + 1;
  const auto en = m_argv.end();
  return std::find(st, en, option) != en;
}

void CmdOptions::get_params(std::vector<std::string>& params) const
{
  for (size_t i = 1; i < m_argv.size(); i++)
    if (m_argv[i].size() > 0)
      if (m_argv[i][0] != '-')
        params.push_back(m_argv[i]);
      else if (m_argv[i][1] != '-') // Assume something like '-o' is always followed by its parameter, but '--diff' is not.
        i++; // skip next arg
}
