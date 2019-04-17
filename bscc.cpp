// Copyright (C) 2017, Fakulta Informatiky Masarykovy univerzity
//
// This file is a part of Seminator, a tool for semi-determinization of omega automata.
//
// Seminator is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Seminator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <bscc.hpp>

bool is_bottom_scc(const_aut_ptr aut,
                   unsigned scc,
                   spot::scc_info* si)
{
  assert(si);
  for (auto s: si->states_of(scc))
    for (auto& t: aut->out(s))
      if (si->scc_of(t.dst) != scc)
        return false;

  return true;
}

bool is_bottom_scc(const_aut_ptr aut, unsigned scc)
{
  auto si = new spot::scc_info(aut);
  bool res = is_bottom_scc(aut, scc, si);
  delete si;
  return res;
}

void print_scc_info(const_aut_ptr aut)
{
  auto si = spot::scc_info(aut);
  unsigned nc = si.scc_count();
  for (unsigned scc = 0; scc < nc; ++scc)
  {
    std::cout << "SCC " << scc << " is bottom: " <<
                  is_bottom_scc(aut, scc, &si) << "\n  ";
    for (auto s: si.states_of(scc))
      std::cout << s << " ";
    std::cout << "\n\n";
  }
  std::cout.flush();
}