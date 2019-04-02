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

#include <seminator.hpp>

#include <spot/parseaut/public.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/stripacc.hh>
#include <spot/twaalgos/degen.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/sccinfo.hh>
#include <spot/misc/optionmap.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twa/bddprint.hh>

int main(int argc, char* argv[])
{
    // Setting up the configurations flags for the user.

    std::string automata_from_cin;
    if (!isatty(fileno(stdin)))
    {
        // don't skip the whitespace while reading
        std::cin >> std::noskipws;

        // use stream iterators to copy the stream to a string
        std::istream_iterator<char> it(std::cin);
        std::istream_iterator<char> end;
        std::string result(it, end);
        automata_from_cin.append(result);
    }

    // Declaration for input options. The rest is in seminator.hpp
    // as they need to be included in other files.
    bool deterministic_first_component = false;
    bool optimize = true;
    bool cd_check = false;
    bool via_tgba = false;
    bool via_tba = false;
    bool via_sba = false;
    unsigned preferred_output = TGBA;
    std::string path_to_file;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // Transformation types
        if (arg.compare("--via-sba") == 0)
            via_sba = true;

        else if (arg.compare("--via-tba") == 0)
            via_tba = true;

        else if (arg.compare("--via-tgba") == 0)
            via_tgba = true;

        else if (arg.compare("--is-cd") == 0)
            cd_check = true;

        // Cut edges
        else if (arg.compare("--cut-on-SCC-entry") == 0) {
            cut_on_SCC_entry = true;

        } else if (arg.compare("--cut-always") == 0)
            cut_always = true;

        else if (arg.compare("--powerset-on-cut") == 0)
            powerset_on_cut = true;

        // Optimizations
        else if (arg.compare("--powerset-for-weak") == 0)
            powerset_for_weak = true;

        else if (arg.compare("--scc0") == 0)
            scc_aware = false;
        else if (arg.compare("--no-scc-aware") == 0)
            scc_aware = false;
        else if (arg.compare("--scc-aware") == 0)
            scc_aware = true;

        else if (arg.compare("-s0") == 0)
            optimize = false;

        else if (arg.compare("--no-reductions") == 0)
            optimize = false;

        // Prefered output
        else if (arg.compare("--cd") == 0)
            deterministic_first_component = true;

        else if (arg.compare("--sd") == 0)
            deterministic_first_component = false;

        else if (arg.compare("--ba") == 0)
            preferred_output = BA;

        else if (arg.compare("--tba") == 0)
            preferred_output = TBA;

        else if (arg.compare("--tgba") == 0)
            preferred_output = TGBA;

        else if (arg.compare("-f") == 0)
        {
            if (argc < i + 1)
            {
                std::cerr << "Seminator: Option requires an argument -- 'f'" << std::endl;
                return 1;
            }
            else
            {
                path_to_file = argv[i+1];
                i++;
            }
        }
        else if (arg.compare("--help") == 0)
        {
            print_help();
            return 1;
        }
        else if (arg.compare("--version") == 0)
        {
            std::cout << "Seminator (" << VERSION_TAG << ")" << std::endl;

            std::cout << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>." << std::endl;
            std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
            std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

            std::cout << std::endl;

            return 0;
        }
        // removed
        else if (arg.compare("--cy") == 0)
        {
            std::cerr << "Invalid option --cy. Use --via-sba -s0 instead.";
            return 2;
        }
        else if (path_to_file.empty())
            path_to_file = argv[i];
    }

    if (!(via_sba || via_tba || via_tgba)) {
      via_sba = true; via_tba = true; via_tgba = true;
    }

    if (automata_from_cin.empty() && path_to_file.empty())
    {
        std::cerr << "Seminator: No automaton to process?  Run 'seminator --help' for help." << std::endl;
        print_usage(std::cerr);
        return 1;
    }

    auto dict = spot::make_bdd_dict();

    spot::parsed_aut_ptr parsed_aut;

    if (!path_to_file.empty())
        parsed_aut = parse_aut(path_to_file, dict);
    else
    {
        const std::string filename = "Reading from std::cin pipe";
        spot::automaton_stream_parser parser(automata_from_cin.c_str(), filename);
        parsed_aut = parser.parse(dict);
    }

    if (!parsed_aut->errors.empty() || parsed_aut->aborted)
    {
        std::cerr << "Seminator: Failed to read automaton from " << path_to_file << std::endl;
        return 1;
    }

    aut_ptr aut = parsed_aut->aut;
    aut_ptr result_sba = nullptr,
            result_tba = nullptr,
            result_tgba = nullptr, result;
    unsigned sba_states  = std::numeric_limits<unsigned>::max();
    unsigned tba_states  = std::numeric_limits<unsigned>::max();
    unsigned tgba_states = std::numeric_limits<unsigned>::max();
    try
    {
        if (cd_check) {
            std::cout << is_cut_deterministic(aut) << std::endl;
            return 0;
        }

        if (via_sba) {
            auto sba_aut = spot::degeneralize(aut);
            result_sba = buchi_to_semi_deterministic_buchi(sba_aut, deterministic_first_component, optimize, preferred_output);
            sba_states = result_sba->num_states();
        }
        if (via_tba) {
            auto tba_aut = spot::degeneralize_tba(aut);
            result_tba = buchi_to_semi_deterministic_buchi(tba_aut, deterministic_first_component, optimize, preferred_output);
            tba_states = result_tba->num_states();
        }
        if (via_tgba) {
            result_tgba = buchi_to_semi_deterministic_buchi(aut, deterministic_first_component, optimize, preferred_output);
            tgba_states = result_tgba->num_states();
        }
        result = (sba_states < tba_states) ? result_sba : result_tba;
        if (!result || (result->num_states() >= tgba_states)) {
            result = result_tgba;
        }

        auto old_n = aut->get_named_prop<std::string>("automaton-name");
        if (old_n)
        {
          std::stringstream ss;
          ss << (deterministic_first_component ? "cDBA for " : "sDBA for ") << *old_n;
          std::string * name = new std::string(ss.str());
          result->set_named_prop("automaton-name", name);
        }

        spot::print_hoa(std::cout, result) << '\n';
        return 0;
    }
    catch (const not_tgba_exception& e)
    {
        std::cerr << "Seminator: The tool requires a TGBA on input" << std::endl;
        return 1;
    }
    catch (const mismatched_bdd_dict_exception& e)
    {
        std::cerr << "Seminator: Mismatched BDD dict" << std::endl;
        return 1;
    }
    catch (const not_semi_deterministic_exception& e)
    {
        std::cerr << "Seminator: The tool could not properly semi-determinize the automata" << std::endl;
    }
    catch (const not_cut_deterministic_exception& e)
    {
        std::cerr << "Seminator: The tool could not properly cut-determinize the automata" << std::endl;
    }
}

aut_ptr buchi_to_semi_deterministic_buchi(aut_ptr aut, bool deterministic_first_component, bool optimization, unsigned output)
{
    // Remove dead and unreachable states and prune accepting conditions in non-accepting SCCs.
    aut = spot::scc_filter(aut, true);

    aut_ptr result;

    // Check if input is TGBA
    if (!aut->acc().is_generalized_buchi())
      throw not_tgba_exception();

    // Check if automaton is deterministic already.
    else if (spot::is_deterministic(aut))
      result = aut;

    // Safety automata can be determinized using powerset construction
    else if (aut->acc().is_all()) {
      result = spot::tba_determinize(aut);
      result->set_acceptance(0, spot::acc_cond::acc_code::t());
    }

    // Check if semi-deterministic already.
    else if (spot::is_semi_deterministic(aut))
    {
      if (deterministic_first_component)
      {
        auto non_det_states = new state_set;
        if (is_cut_deterministic(aut, non_det_states))
          result = aut;
        else
          result = determinize_first_component(aut, non_det_states);
        delete non_det_states;
      }
      else
        result = aut;
    }
    else
    {   // Use the breakpoint construction
        bp_twa resbp(aut, deterministic_first_component,
          powerset_for_weak, powerset_on_cut, scc_aware, cut_condition);
        result = resbp.res_aut();
    }

    // Optimization
    // Purge dead and unreachable states.
    result->purge_dead_states();

    if (optimization)
    {

        spot::postprocessor postprocessor;
        if (deterministic_first_component)
        {
          static spot::option_map extra_options;
          extra_options.set("ba_simul",1);
          extra_options.set("simul",1);
          postprocessor = spot::postprocessor(&extra_options);
        }
        result = postprocessor.run(result);
    }

    if (output == TBA && result->acc().is_generalized_buchi())
        result = spot::degeneralize_tba(result);
    else if (output == BA)
        result = spot::degeneralize(result);

    if (!spot::is_semi_deterministic(result))
        throw not_semi_deterministic_exception();
    else if (deterministic_first_component && !is_cut_deterministic(result))
        throw not_cut_deterministic_exception();

    return result;
}

bool is_cut_deterministic(const_aut_ptr aut, std::set<unsigned>* non_det_states)
{
    unsigned UNKNOWN = 0;
    unsigned IN_CUT = 1;
    unsigned NOT_IN_CUT = 2;

    // Take the basic semi-determinism check as implemented in SPOT.
    spot::scc_info si(aut);
    si.determine_unknown_acceptance();
    unsigned nscc = si.scc_count();
    assert(nscc);
    std::vector<bool> reachable_from_acc(nscc);

    std::vector<unsigned> cut(si.scc_count());

    bool cut_det = true;

    do // iterator of SCCs in reverse topological order
    {
        --nscc;
        if (si.is_accepting_scc(nscc) || reachable_from_acc[nscc])
        {
            cut[nscc] = IN_CUT;

            for (unsigned succ: si.succ(nscc))
                reachable_from_acc[succ] = true;

            for (unsigned src: si.states_of(nscc))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                    if (!bdd_implies(t.cond, available)) // Not even semi-deterministic.
                      cut_det = false;
                    else
                      available -= t.cond;
            }
        }
    }
    while (nscc);

    for (unsigned i = 0; i < si.scc_count(); i++)
    {
        if (!si.is_accepting_scc(i) && !reachable_from_acc[i])
        {
            for (unsigned succ: si.succ(i))
              if (cut[succ] == NOT_IN_CUT)
                  cut[i] = NOT_IN_CUT;

            std::set<unsigned> edge_states;

            // Check determinism of this component.
            for (unsigned src: si.states_of(i))
            {
                bdd available = bddtrue;
                for (auto& t: aut->out(src))
                {
                    // We check whether the state is at the edge of the SCC.
                    if (si.scc_of(t.dst) != i)
                      edge_states.insert(src);
                    else if (!bdd_implies(t.cond, available))
                      cut_det = false;// SCC not det. => automaton not cut-det.
                    else
                      available -= t.cond;
                }
            }

            if (cut[i] == UNKNOWN)
            {
                bool is_in_cut = true;
                // Inner part of SCC deterministic, what about the outgoing edges?
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                      if (!bdd_implies(t.cond, available))
                        is_in_cut = false;
                      else
                        available -= t.cond;
                }

                cut[i] = is_in_cut ? IN_CUT : NOT_IN_CUT;
            }
            else if (cut_det)
            {
                // SCC cannot be in the cut, check if transitions outside cut
                // are deterministic.
                for (unsigned edge_state : edge_states)
                {
                    bdd available = bddtrue;
                    for (auto& t: aut->out(edge_state))
                        if (cut[si.scc_of(t.dst)] != IN_CUT) {
                            if (!bdd_implies(t.cond, available))
                                cut_det = false;
                            else
                                available -= t.cond;
                        }
                }
            }
        }
        else
            cut[i] = IN_CUT;
    }

    if (non_det_states != nullptr)
        for (unsigned scc = 0; scc < cut.size(); scc++)
            if (cut[scc] != IN_CUT)
                for (unsigned state: si.states_of(scc))
                    non_det_states->insert(state);

    return cut_det;
}

aut_ptr determinize_first_component(const_aut_ptr src, state_set * to_determinize)
{
  auto res = spot::make_twa_graph(src->get_dict());
  res->copy_ap_of(src);
  res->set_acceptance(src->get_acceptance());
  auto names = new std::vector<std::string>;

  // Setup the powerset construction
  auto ps2num = std::unique_ptr<power_map>(new power_map);
  auto num2ps = std::unique_ptr<succ_vect>(new succ_vect);
  auto psb = std::unique_ptr<powerset_builder>(new powerset_builder(src));

  // returns the state`s index, creates a new state if needed
  auto get_state = [&](state_set ps) {
    if (ps2num->count(ps) == 0)
    {
      // create a new state
      assert(num2ps->size() == res->num_states());
      num2ps->emplace_back(ps);
      auto state = res->new_state();
      (*ps2num)[ps] = state;
      //TODO add to bp1 states

      names->emplace_back(powerset_name(&ps));
      return state;
    } else
      return ps2num->at(ps);

  };

  // Set the initial state
  state_t init_num = src->get_init_state_number();
  state_set ps{init_num};
  res->set_init_state(get_state(ps));

  // Compute powerset with respect to to_determinize
  for (state_t s = 0; s < res->num_states(); ++s)
  {
    auto ps = num2ps->at(s);
    auto succs = std::unique_ptr<succ_vect>(psb->get_succs(&ps,
                                            to_determinize->begin(),
                                            to_determinize->end()));
    for(size_t c = 0; c < psb->nc_; ++c)
    {
      auto cond = psb->num2bdd_[c];
      auto d_ps = succs->at(c);
      // Skip transitions to ∅
      if (d_ps == empty_set)
        continue;
      auto dst = get_state(d_ps);
      res->new_edge(s, dst, cond);
    }
  }

  // remeber for later stop iteration when adding cut transitions
  auto lsize = res->num_states();

  // Copy the second part
  typedef std::map<state_t, state_t> state_map;
  state_map old2new;
  state_map new2old;
  for (state_t s = 0; s < src->num_states(); s++)
  {
    if (to_determinize->count(s) > 0) // skip states from the 1st component
      continue;
    auto ns = res->new_state();
    old2new[s] = ns;
    new2old[ns] = s;
    names->emplace_back(std::to_string(s));
  }
  // Now copy the transitions
  for (state_t s = 0; s < src->num_states(); s++)
  {
    if (to_determinize->count(s) > 0) // skip states from the 1st component
      continue;
    for (auto e : src->out(s))
      res->new_edge(old2new[e.src],old2new[e.dst],e.cond,e.acc);
  }

  for (state_t ns = 0; ns < lsize; ns++)
  {
    auto ps = num2ps->at(ns);
    auto succs = std::unique_ptr<succ_vect>(psb->get_succs(&ps,
                                            to_determinize->begin(),
                                            to_determinize->end(),
                                            true
                                           ));
    for(size_t c = 0; c < psb->nc_; ++c)
    {
      auto cond = psb->num2bdd_[c];
      auto d_ps = succs->at(c);
      // Skip transitions to ∅
      if (d_ps == empty_set)
        continue;
      for (auto s : d_ps)
        res->new_edge(ns, old2new[s], cond);
    }
  }


  res->merge_edges();
  res->set_named_prop("state-names", names);
  return res;
}
