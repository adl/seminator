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
#pragma once

#include <utility>
#include <stdexcept>
#include <unistd.h>

#include <types.hpp>
#include <breakpoint_twa.hpp>

#include <spot/misc/bddlt.hh>


static const std::string VERSION_TAG = "v1.2.0dev";

static const unsigned TGBA = 0;
static const unsigned TBA = 1;
static const unsigned BA = 2;

bool jump_enter = false;
bool jump_always = false;

// TO REMOVE!!!
typedef std::tuple<int, state_set, state_set, unsigned> todo_state;
typedef std::vector<todo_state> todo_list;
typedef std::map<state_set, unsigned> powerset_state_dictionary;
typedef std::tuple<state_set, unsigned> powerset_todo_state;
typedef std::vector<powerset_todo_state> powerset_todo_list;
typedef std::map<unsigned,unsigned> state_map;
// TO REMOVE
#include <stack>




/**
 * The semi-determinization algorithm as thought of by F. Blahoudek, J. Strejcek and M. Kretinsky.
 *
 * @param[in] aut TGBA to transform to a semi-deterministic TBA.
 */
spot::twa_graph_ptr buchi_to_semi_deterministic_buchi(spot::twa_graph_ptr& aut, bool deterministic_first_component, bool optimization, unsigned output);

/**
 * Helper function to convert an edge condition to a vector of all the different minterms.
 *
 * @param[in] allap         All atomic propositions used through out the automata.
 * @param[in] cond          Edge condition.
 * @param[in, out] minterms A map of edge conditions to minterms that we have already found.
 * @return Vector of minterms.
 */
std::vector<bdd> edge_condition_to_minterms(bdd allap, bdd cond, std::map<bdd, std::vector<bdd>, spot::bdd_less_than>* minterms);

/**
 * Helper function that pairs a set of states an already existing (or new, in the case it does not exist) state in the powerset construction of an automata.
 *
 * @param[in,out] aut The powerset automata.
 * @param[in,out] sdict Dictionary keeping track of the relationship between pair of sets of states and new states in the new automata.
 * @param[out]    todo Vector of states we have to go through next.
 * @param[out]    names Vector of names that we keep to name the states of the new automata.
 * @param[in]     states Left set of states.
 * @return Returns a number of the state that either already existed or was created a new representing the pair of sets of states from the old automata.
 */
unsigned powerset_set_to_state(spot::twa_graph_ptr aut, powerset_state_dictionary* sdict, powerset_todo_list* todo, std::vector<std::string>* names, state_set states);


void determinize_first_component(spot::twa_graph_ptr result, spot::twa_graph_ptr aut, std::set<unsigned> to_determinize);


/**
 * A function that checks whether a given automaton is cut-deterministic.
 *
 * @param[in] aut               The automata to check for cut-determinism.
 * @param[out] non_det_states   Vector of the states that block cut-determinism.
 * @return Whether the automaton is cut-deterministic or not.
 */
bool is_cut_deterministic(const spot::twa_graph_ptr& aut, std::set<unsigned>* non_det_states = nullptr);

/**
* Returns whether a cut transition (jump to the deterministic component) for the
* current edge should be created.
*
* @param[in] aut                The input automaton_stream_parser
* @param[in] e                  The edge beeing processed
* @return True if some jump condition is satisfied
*
* Currently, 4 conditions trigger the jump:
*  1. If the input automaton is safety (accepts all)
*  2. If the edge has the highest mark
*  3. If we freshly enter accepting scc (--jump-enter only)
*  4. If e leads to accepting SCC (--jump-always only)
*/
bool jump_condition(const_aut_ptr, spot_edge);

/**
 * Class representing an exception thrown when the algorithm is not run on a proper SBwA automata.
 */
class not_tgba_exception: public std::runtime_error {
public:
    not_tgba_exception()
        : std::runtime_error("algorithm excepts a TGBA")
        {}
};

/**
 * Class representing an exception thrown when trying to copy to an automata with a different BDD dictionary.
 */
class mismatched_bdd_dict_exception: public std::runtime_error {
public:
    mismatched_bdd_dict_exception()
        : std::runtime_error("the source and destination automata require to have the same BDD dictionary")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not semi-deterministic (this, of course, should not happen).
 */
class not_semi_deterministic_exception: public std::runtime_error {
public:
    not_semi_deterministic_exception()
        : std::runtime_error("the resulting automata is not semi-deterministic")
        {}
};

/**
 * Class representing an exception thrown when the resulting automata is not cut-deterministic (this, of course, should not happen).
 */
class not_cut_deterministic_exception: public std::runtime_error {
public:
    not_cut_deterministic_exception()
        : std::runtime_error("the resulting automata is not cut-deterministic")
        {}
};
