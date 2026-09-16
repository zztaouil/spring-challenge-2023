#pragma GCC optimize("Ofast","unroll-loops", "omit-frame-pointer", "inline")
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target("rdrnd", "popcnt", "avx", "bmi2")
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <ctime>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <random>
#include <cassert>
#include <cmath>
#include <string.h> 
#include <set>
#include <unordered_map>

using namespace std;

namespace game_data {
    struct  cell
    {
        int     type;
        int     resources;
        int     beacon;
        int     my_ants;
        int     opp_ants;
    };
    struct  t_data
    {
        cell     *gp_info;
        int     **gp;
        int     **d_mat;
        int     *my_bases;
        int     *opp_bases;
        int     number_of_bases;
        int     number_of_cells;
        int     round;
        int     my_crystal;
        int     opp_crystal;
        int     total_crystal;
        int     total_eggs;
        int     crystal;
        int     eggs;
        int     initial_ants;
        int     my_ants;
        int     *s_zero;
        int     *last_move;
        long    start;
    };

    t_data     *data;

    int find_shortest_distance(int a, int b) {
        queue<int> queue;
        vector<int> path;
        unordered_map<int, int> prev;
        int i1, visited, tmp, head = 0;
    
        prev.insert({a, -1});
        queue.push(a);
        while (!queue.empty()) {
            if (prev.count(b) > 0)
                break;
            head = queue.front();
            queue.pop();
            for (i1 = 0; i1 < 6; i1++) {
                tmp = game_data::data->gp[head][i1];
                if (tmp != -1){
                    visited = prev.count(tmp);
                    if (!visited) {
                        prev.insert({tmp, head});
                        queue.push(tmp);
                    }   
                }
            }
        }
        if (!prev.count(b))
            return -1;
        head = b;
        while (head != -1) {
            path.insert(path.begin(), head);
            unordered_map<int, int>::iterator it = prev.find(head);
            assert(it != prev.end());
            head = it->second;
        }
        return path.size();
    }

    void    calculate_crystal(void) {
        int     total_crystal, total_eggs, i1;

        total_crystal = 0;
        total_eggs = 0;
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            total_crystal += game_data::data->gp_info[i1].resources * (game_data::data->gp_info[i1].type == 2);
            total_eggs += game_data::data->gp_info[i1].resources * (game_data::data->gp_info[i1].type == 1);
        }
        game_data::data->total_crystal = total_crystal;
        game_data::data->total_eggs = total_eggs;
    }

    void    pre_calculation(void)
    {
        int i1, i2, tmp;

        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            for (i2 = i1 + 1; i2 < game_data::data->number_of_cells; i2++) {
                data->d_mat[i1][i2] = -1;
                data->d_mat[i2][i1] = -1;
                tmp = game_data::find_shortest_distance(i1, i2);
                data->d_mat[i1][i2] = tmp;
                data->d_mat[i2][i1] = tmp;
                if (!i1) {
                    game_data::data->initial_ants += game_data::data->gp_info[i2].my_ants;
                }
            }
        }
        calculate_crystal();

    }

}
int check_time(void) {
    struct timeval  now;
    int             k;

    gettimeofday(&now, NULL);
    k = (now.tv_sec * 1000) + (now.tv_usec / 1000);
    k = k - game_data::data->start;
    return k;
}
namespace refere {
    struct  ant_allocation {
        int ant_idx;
        int beacon_idx;
        int amount;
    };
    struct  ant_cell {
        int idx;
        int nb;
    };
    struct beacon_cell {
        int idx;
        int strength;
        int wiggle;
    };
    struct ant_pair {
        refere::ant_cell    *ant_cell;
        refere::beacon_cell *beacon_cell;
    };
    struct move {
        int from;
        int to;
        int amount;
    };
    struct sa_s {
        int idx;
        int strength;
    };
    struct  game_state {
        game_data::cell     *gp_info;
        int                 crystal;
    };
    struct custom_sort1 {
        custom_sort1(void){};
        bool operator() (ant_pair i,ant_pair j) {
            if (game_data::data->d_mat[i.ant_cell->idx][i.beacon_cell->idx] != game_data::data->d_mat[j.ant_cell->idx][j.beacon_cell->idx])
                return game_data::data->d_mat[i.ant_cell->idx][i.beacon_cell->idx] < game_data::data->d_mat[j.ant_cell->idx][j.beacon_cell->idx];
            if (i.ant_cell->idx != j.ant_cell->idx)
                return i.ant_cell->idx < j.ant_cell->idx;
            return i.beacon_cell->idx < j.beacon_cell->idx;
        }
    };

    struct custom_sort2 {
        int     _target;
        custom_sort2(int target): _target(target){};
        bool operator() (int a, int b) {
            if (a == -1 || b == -1)
                return a > b;
            if (game_data::data->d_mat[a][_target] != game_data::data->d_mat[b][_target])
                return game_data::data->d_mat[a][_target] < game_data::data->d_mat[b][_target];
            if (game_data::data->gp_info[a].my_ants != game_data::data->gp_info[b].my_ants)
                return game_data::data->gp_info[a].my_ants > game_data::data->gp_info[b].my_ants;
            if (game_data::data->gp_info[a].beacon != game_data::data->gp_info[b].beacon)
                return game_data::data->gp_info[a].beacon > game_data::data->gp_info[b].beacon;
            return a < b;
        }
    };

    ant_allocation     *allocate_ants(ant_cell **ant_cells, beacon_cell **beacon_cells, int *allocations_size) {
        ant_allocation  *allocations;
        ant_pair        *all_pairs;
        double  scaling_factor;
        int     i1, i2, i3, stragglers, ant_sum, ant_nb, beacon_nb, beacon_sum, total_pairs;
        long     high_beacon_value, low_beacon_value;
        int     ant_count, beacon_count, wiggle_room, max_alloc, remaining_pairs;

        ant_sum = 0;
        ant_nb = 0;
        beacon_sum = 0;
        beacon_nb = 0;
        for (i1 = 0; ant_cells[i1]->idx != -1; i1++) {
            // fprintf(stderr, "ant_cell: (%d, %d) addr %p\n", ant_cells[i1]->idx, ant_cells[i1]->nb, ant_cells[i1]);
            ant_sum += ant_cells[i1]->nb;
            ant_nb++;
        }
        for (i1 = 0; beacon_cells[i1]->idx != -1; i1++) {
            // fprintf(stderr, "beacon_cell: (%d, %d)\n", beacon_cells[i1]->idx, beacon_cells[i1]->strength);
            beacon_sum += beacon_cells[i1]->strength;
            beacon_nb++;
        }
        total_pairs = ant_nb * beacon_nb;
        remaining_pairs = total_pairs;
        allocations = new ant_allocation[total_pairs * 2];
        all_pairs = new ant_pair[total_pairs + 1];
        scaling_factor = (double) ant_sum / beacon_sum;

        for (i1 = 0; i1 < beacon_nb; i1++) {
            high_beacon_value = ceil(beacon_cells[i1]->strength * scaling_factor);
            low_beacon_value = beacon_cells[i1]->strength * scaling_factor;
            beacon_cells[i1]->strength = max((long)1, low_beacon_value);
            beacon_cells[i1]->wiggle = high_beacon_value - beacon_cells[i1]->strength;
        }
        // fprintf(stderr, "pairs number: %d\n", total_pairs);
        for (i1 = 0, i3 = 0; i1 < ant_nb; i1++) {
            for (i2 = 0; i2 < beacon_nb; i2++) {
                if (ant_cells[i1]->idx != -1 && beacon_cells[i2]->idx != -1 && game_data::data->d_mat[ant_cells[i1]->idx][beacon_cells[i2]->idx] != -1) {
                    all_pairs[i3].ant_cell = ant_cells[i1];
                    all_pairs[i3++].beacon_cell = beacon_cells[i2];
                    // fprintf(stderr, "all_pairs: (%d, %d) addr: %p\n", all_pairs[i3 - 1].ant_cell->idx,
                    //     all_pairs[i3 - 1].beacon_cell->idx, &all_pairs[i3 - 1]);
                }
            }
        }
        sort(all_pairs, all_pairs + total_pairs, custom_sort1());
        // for (i1 = 0; i1 < total_pairs; i1++) {
        //     fprintf(stderr, "sorted_pairs: (%d, %d)\n", all_pairs[i1].ant_cell->idx, all_pairs[i1].beacon_cell->idx);
        // }
        stragglers = 0;
        i2 = 0;
        while (remaining_pairs > 0)
        {
            // fprintf(stderr, "%d pair remaining\n", remaining_pairs);
            for (i1 = 0; i1 < total_pairs; i1++) {
                // fprintf(stderr, "1) all_pairs + %d: %p, *(all_pairs + %d).ant_cell: %p\n", i1, &all_pairs[i1], i1,
                    // all_pairs[i1].ant_cell);
                if (all_pairs[i1].ant_cell->nb <= 0) {
                    continue ;
                }
                ant_count = all_pairs[i1].ant_cell->nb;
                beacon_count = all_pairs[i1].beacon_cell->strength;
                wiggle_room = all_pairs[i1].beacon_cell->wiggle;
                max_alloc = (int) (stragglers ? min(ant_count, beacon_count + wiggle_room) :
                    min(ant_count, beacon_count));
                
                if (max_alloc > 0) {
                    allocations[i2++] = {all_pairs[i1].ant_cell->idx, all_pairs[i1].beacon_cell->idx,
                        max_alloc};
                    all_pairs[i1].ant_cell->nb -= max_alloc;
                    if (!stragglers) {
                        all_pairs[i1].beacon_cell->strength -= max_alloc;
                    } else {
                        all_pairs[i1].beacon_cell->strength -= (max_alloc - wiggle_room);
                        all_pairs[i1].beacon_cell->wiggle = 0;
                    }
                    // fprintf(stderr, "assigned %d from %d to %d\n", max_alloc, all_pairs[i1].ant_cell->idx, all_pairs[i1].beacon_cell->idx);
                }
            }
            // fprintf(stderr, "$\n");
            // for (i1 = 0; i1 < total_pairs; i1++)
                // fprintf(stderr, "2) all_pairs + %d: %p, *(all_pairs + %d).ant_cell: %p\n", i1, &all_pairs[i1], i1, all_pairs[i1].ant_cell);
            for (i1 = 0; i1 < total_pairs; i1++) {
                // fprintf(stderr, "remaining_pair: (%d, %d), number: %d\n", all_pairs[i1].ant_cell->idx, all_pairs[i1].beacon_cell->idx, all_pairs[i1].ant_cell->nb);
                if (all_pairs[i1].ant_cell->nb <= 0) {
                    remaining_pairs--;
                }
            }
            stragglers = 1;
        }
        allocations[i2].ant_idx = -1;
        *allocations_size = i2;
        delete [] all_pairs;
        return allocations;
    }

    int             find_next_move(int ant_idx, int beacon_idx, int amount) {
        vector<int> vec;
        int i1;

        if (ant_idx == beacon_idx)
            return -1;
        sort(game_data::data->gp[ant_idx], game_data::data->gp[ant_idx] + 6, custom_sort2(beacon_idx));
        // for (i1 = 0; i1 < 6; i1++) {
        //     fprintf(stderr, "%d ", game_data::data->gp[ant_idx][i1]);
        // }
        // fprintf(stderr, "\n");
        assert(game_data::data->gp[ant_idx][0] > -1);
        return game_data::data->gp[ant_idx][0];
    }

    refere::move   *predict_ant_position(int  *s, refere::game_state *c_s) {
        ant_cell        **ant_cells;
        beacon_cell     **beacon_cells;
        ant_allocation  *allocations;
        move            *moves;
        int             *path, i1, i2, next, allocations_size, path_size;
        
        moves = 0;
        ant_cells = new ant_cell *[game_data::data->number_of_cells];
        beacon_cells = new beacon_cell *[game_data::data->number_of_cells];
        for (i1 = 0, i2 = 0; i1 < game_data::data->number_of_cells; i1++)
        {
            if (c_s->gp_info[i1].my_ants > 0)
            {
                ant_cells[i2] = new ant_cell;
                ant_cells[i2]->idx = i1;
                ant_cells[i2++]->nb = c_s->gp_info[i1].my_ants;
            }
        }
        ant_cells[i2] = new ant_cell;
        ant_cells[i2]->idx = -1;
        assert(s);
        i1 = 0;
        i2 = 0;
        for (; i1 < game_data::data->number_of_cells; i1++)
        {
            assert(i2 < game_data::data->number_of_cells);
            if (!s[i1])
                continue ;
            beacon_cells[i2] = new beacon_cell;
            beacon_cells[i2]->idx = i1;
            beacon_cells[i2]->strength = s[i1];
            beacon_cells[i2++]->wiggle = 0;
        }
        beacon_cells[i2] = new beacon_cell;
        beacon_cells[i2]->idx = -1;
        allocations = allocate_ants(ant_cells, beacon_cells, &allocations_size);
        // fprintf(stderr, "out alloc %p\n", allocations);
        moves = new move[allocations_size + 1];
        i2 = 0;
        for (i1 = 0; allocations[i1].ant_idx != -1; i1++) {
            // fprintf(stderr, "allocation: (%d, %d, %d), %p\n", allocations[i1].ant_idx, allocations[i1].beacon_idx,
                // allocations[i1].amount, &allocations[i1]);
            next = find_next_move(allocations[i1].ant_idx, allocations[i1].beacon_idx, allocations[i1].amount);
            if (next > -1) {
                moves[i2++] = {allocations[i1].ant_idx, next, allocations[i1].amount};
            }
            // fprintf(stderr, "finished handling allocation (%d, %d, %d)\n", allocations[i1].ant_idx, allocations[i1].beacon_idx,
            //     allocations[i1].amount);
        }
        moves[i2].from = -1;
        for (i1 = 0; ant_cells[i1]->idx != -1; i1++) {
            delete ant_cells[i1];
        }
        for (i1 = 0; beacon_cells[i1]->idx != -1; i1++) {
            delete beacon_cells[i1];
        }
        delete [] ant_cells;
        delete [] beacon_cells;
        delete [] allocations;
        assert(moves);
        return moves;
    }
}

namespace sim {

    refere::game_state  *init_game_state(game_data::cell const *gp_data) {
        refere::game_state  *g_s;
        int         i1;

        g_s = new refere::game_state;
        g_s->gp_info = new game_data::cell [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            g_s->gp_info[i1].beacon = gp_data[i1].beacon;
            g_s->gp_info[i1].my_ants = gp_data[i1].my_ants;
            g_s->gp_info[i1].opp_ants = gp_data[i1].opp_ants;
            g_s->gp_info[i1].resources = gp_data[i1].resources;
            g_s->gp_info[i1].type = gp_data[i1].type;
        }
        g_s->crystal = 0;
        return g_s;
    }
    pair<int, int>  *calculate_enemy_flow(refere::game_state *next_state) {
        priority_queue<pair<int, int> > q;
        pair<int, int>                  current, *flow;
        int                             income, i1, neigh;

        income = 0;
        flow = new pair<int, int> [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            flow[i1] = {-1, -1};
        }
        for (i1 = 0; i1 < game_data::data->number_of_bases; i1++) {
            q.push({next_state->gp_info[game_data::data->opp_bases[i1]].opp_ants, game_data::data->opp_bases[i1]});
            flow[game_data::data->opp_bases[i1]].second = next_state->gp_info[game_data::data->opp_bases[i1]].opp_ants;
        }
        while (!q.empty()) {
            current = q.top(); q.pop();
            for (i1 = 0; i1 < 6; i1++) {
                neigh = game_data::data->gp[current.second][i1];
                if (neigh != -1 && flow[neigh].second == -1 && next_state->gp_info[neigh].opp_ants > 0) {
                    flow[neigh].second = min(next_state->gp_info[neigh].opp_ants, current.first);
                    q.push({flow[neigh].second, neigh});
                }
            }
        }
        return flow;
    }
    void           simulate_my_eggs_income(refere::game_state  *next_state, pair<int, int>  *flow) {
        priority_queue<pair<int, int> > q;
        pair<int, int>                  current;
        int                             income, i1, neigh;

        income = 0;
        for (i1 = 0; i1 < game_data::data->number_of_bases; i1++) {
            q.push({next_state->gp_info[game_data::data->my_bases[i1]].my_ants, game_data::data->my_bases[i1]});
            flow[game_data::data->my_bases[i1]].first = next_state->gp_info[game_data::data->my_bases[i1]].my_ants;
        }
        while (!q.empty()) {
            current = q.top(); q.pop();
            if (flow[current.second].first >= flow[current.second].second) {
                income += min(current.first, next_state->gp_info[current.second].resources * (next_state->gp_info[current.second].type == 1));
            }
            for (i1 = 0; i1 < 6; i1++) {
                neigh = game_data::data->gp[current.second][i1];
                if (neigh != -1 && flow[neigh].first == -1 && next_state->gp_info[neigh].my_ants > 0) {
                    flow[neigh].first = min(next_state->gp_info[neigh].my_ants, current.first);
                    q.push({flow[neigh].first, neigh});
                }
            }
        }
        for (i1 = 0; i1 < game_data::data->number_of_bases; i1++) {
            next_state->gp_info[game_data::data->my_bases[i1]].my_ants += income;
        }
    }

    void           simulate_my_crystal_income(refere::game_state  *next_state, pair<int, int>   *flow) {
        priority_queue<pair<int, int> > q;
        pair<int, int>                  current;
        int                             income, i1, neigh;

        income = 0;
        for (i1 = 0; i1 < game_data::data->number_of_bases; i1++) {
            q.push({next_state->gp_info[game_data::data->my_bases[i1]].my_ants, game_data::data->my_bases[i1]});
            flow[game_data::data->my_bases[i1]].first = next_state->gp_info[game_data::data->my_bases[i1]].my_ants;
        }
        while (!q.empty()) {
            current = q.top(); q.pop();
            if (flow[current.second].first >= flow[current.second].second) {
                income += min(current.first, next_state->gp_info[current.second].resources * (next_state->gp_info[current.second].type == 2));
            }
            for (i1 = 0; i1 < 6; i1++) {
                neigh = game_data::data->gp[current.second][i1];
                if (neigh != -1 && flow[neigh].first == -1 && next_state->gp_info[neigh].my_ants > 0) {
                    flow[neigh].first = min(next_state->gp_info[neigh].my_ants, current.first);
                    q.push({flow[neigh].first, neigh});
                }
            }
        }
        next_state->crystal = income;
    }

    void          simulate_next_round(int *s, refere::game_state *next_state) {
        refere::move    *moves;
        int             i1, tmp;
        pair<int, int>  *opp_flow;

        assert(s);
        moves = refere::predict_ant_position(s, next_state);

        assert(moves);
        for (i1 = 0; moves[i1].from > -1; i1++) {
            next_state->gp_info[moves[i1].from].my_ants -= moves[i1].amount;
            next_state->gp_info[moves[i1].to].my_ants += moves[i1].amount;
        }
        opp_flow = calculate_enemy_flow(next_state);
        simulate_my_eggs_income(next_state, opp_flow);
        simulate_my_crystal_income(next_state, opp_flow);

        delete [] opp_flow;
        delete [] moves;
        // fprintf(stderr, "sim: gained crystal %d\n", next_state->crystal);
        // for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
        //     tmp = next_state->gp_info[i1].my_ants;
        //     if (tmp > 0) {
        //         fprintf(stderr,"sim: %d ants at %d\n", tmp, i1);
        //     }
        // }
    }
}

namespace game {
    struct custom_sort {
        custom_sort(void){};
        bool operator() (vector<int> &vec1, vector<int> &vec2) {
            return vec1.size() < vec2.size();
        }
    };
    int             connection_scoring(refere::game_state  *s, int start) {
        queue<int>  q;
        int         *visited, i1;
        int         connection_nb, current, tmp;

        visited = new int [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            visited[i1] = 0;
        }
        connection_nb = 0;
        if (s->gp_info[start].my_ants > 0) {
            q.push(start);
            visited[start] = 1;
        }
        while (!q.empty())
        {
            current = q.front();
            q.pop();

            if (s->gp_info[current].my_ants > 0) {
                connection_nb += 1;
            }
            for (i1 = 0; i1 < 6; i1++) {
                tmp = game_data::data->gp[current][i1];
                if (tmp != -1 && !visited[tmp] &&
                    s->gp_info[tmp].my_ants > 0) {
                        visited[tmp] = 1;
                        q.push(tmp);
                    }
            }
        }
        delete [] visited;
        return connection_nb;
    }

    int    *neighbor(int const *s) {
        int     i1, i2, rand_idx;
        int    *n_s, b_present;

        b_present = 0;
        n_s = new int [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            n_s[i1] = s[i1];
            if (n_s[i1] > 0)
                b_present++;
        }
        rand_idx = rand() % b_present;

        for (i1 = 0, i2 = 0; i1 < game_data::data->number_of_cells; i1++) {
            if (n_s[i1] > 0 && i2++ == rand_idx) {
                n_s[i1] = rand() % 3;
            }
        }

        assert(n_s);
        return n_s;
    }

    float  temperature(float i_max, int k) {
        return (float) i_max / (float) (k + 1);
    }

    int           distance_to_resource(refere::game_state *s, int type, int ant_idx) {
        queue<int>  q;
        int         visited[game_data::data->number_of_cells], dist, current;
        int         i1, tmp;

        dist = -1;
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++)
            visited[i1] = 0;
        q.push(ant_idx);
        visited[ant_idx] = 1;
        while (!q.empty()) {
            current = q.front();
            q.pop();
            if (s->gp_info[current].type == type) {
                dist = game_data::data->d_mat[current][ant_idx];
                break ;
            }
            for (i1 = 0; i1 < 6; i1++) {
                tmp = game_data::data->gp[current][i1];
                if (tmp != -1 && !visited[tmp]) {
                    q.push(tmp);
                    visited[tmp] = 1;
                }
            }
        }
        return dist;
     }

    float         fitness(refere::game_state *s) {
        int         i1, c_ants;
        float       score;

        score = 0;
        c_ants = 0;
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            c_ants += s->gp_info[i1].my_ants;
        }
        c_ants = (c_ants - game_data::data->my_ants) / game_data::data->number_of_bases;
        score += c_ants ;
        score += s->crystal; 

        return score;
    }

    int  energy(int *s) {
        refere::game_state     *new_state;
        int                 sim_nb, i1, score;

        sim_nb = 5; // simulate sim_nb round in each SA iteration
        score = 1;
        new_state = sim::init_game_state(game_data::data->gp_info);
        for (i1 = 0; i1 < sim_nb; i1++) {
            sim::simulate_next_round(s, new_state);
            score += fitness(new_state);
        }
        delete [] new_state->gp_info;
        delete new_state;
        return score;
    }

    float  P(float e, float e_prime, float t) {
        if (e_prime < e)
            return 1;
        assert(t);
        return exp(((float)-1 * (e_prime / e)) / t);
    }

    int    *state_zero(refere::game_state *s) {
        int     *s_z;
        int     i1, i2, iteration_nb, rand_idx;
        int     beacons_target;

        if (game_data::data->s_zero != 0x0) {
            return game_data::data->s_zero;
        }
        beacons_target = 10;
        s_z = new int [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells;) {
            s_z[i1++] = 0;
        }
        i2 = beacons_target; // tweakable
        for (i1 = 0; i1 < i2; i1++) {
            s_z[rand() % game_data::data->number_of_cells] = rand() % 2;
        }
        assert(s_z);
        return s_z;
    }

    int *copy_int_arr(int *s) {
        int i1, *s2;

        s2 = new int [game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            s2[i1] = s[i1];
        }
        return s2;
    }

    int *simulated_annealing(int *s_z) {
        refere::game_state *s_game_data;
        struct timeval  now;
        int             *s, *s_prime, *s_best;
        int             k, k_max, i1, *path, path_size;
        int           e, e_prime, e_best;
        float           t;
        int             it;
        float            i_max;

        fprintf(stderr, "simulated annealing called\n");
        s_game_data = sim::init_game_state(game_data::data->gp_info);
        i_max = 6969;
        s = s_z;
        it = 0;
        k_max = (game_data::data->round == 1) ? 999 : 98; // ms
        e = energy(s);
        e_best = e;
        // fprintf(stderr, "entering sa iterations s: %p, e: %f\n", s, e);
        for (;check_time() < k_max;) {
            assert(s);
            s_prime = neighbor(s);
            assert(s_prime);
            e_prime = energy(s_prime);
            if (e_prime > e) {
                delete s;
                s = s_prime;
                e = e_prime;
            }
            it++;
        }
        delete [] s_game_data->gp_info;
        delete s_game_data;
        game_data::data->s_zero = s;
        printf("MESSAGE %di %de %dms;", it, e, check_time());
        fprintf(stderr, "SA returned %p\nit: %d, e: %f\n", s, it, e);
        return s;
    }

    vector<int> find_path(refere::game_state *s, int src)
    {
        vector<int>     path;
        queue<int>      q;
        int             visited[game_data::data->number_of_cells], parent[game_data::data->number_of_cells];
        int             i1, current, tmp;

        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            visited[i1] = 0;
            parent[i1] = 0;
        }
        q.push(src);
        parent[src] = -1;
        visited[src] = 1;
        while (!q.empty()) {
            current = q.front();
            q.pop();

            if (s->gp_info[current].beacon > 0) {
                break ;
            }

            for (i1 = 0; i1 < 6; i1++) {
                tmp = game_data::data->gp[current][i1];
                if (tmp != -1 && !visited[tmp]) {
                    q.push(tmp);
                    parent[tmp] = current;
                    visited[tmp] = 1;
                }
            }
        }

        path.insert(path.end(), current);
        while (current != src) {
            current = parent[current];
            path.insert(path.end(), current);
        }
        return path;
    }

    vector<vector<int> > generate_paths(refere::game_state *s, int type) {
        vector<vector<int> > v;
        vector<int>          p;
        int i1;

        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            if (s->gp_info[i1].type == type) {
                p = find_path(s, i1);
                v.push_back(p);
            }
        }
        return v;
    }

    vector<int>     get_crystal_distrubtion(void) {
        int i1, i2, m_d, o_d;
        vector<int> vec;

        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            m_d = 99999;
            o_d = 99999;
            if (game_data::data->gp_info[i1].type == 2 && game_data::data->gp_info[i1].resources > 0) {
                for (i2 = 0; i2 < game_data::data->number_of_bases; i2++) {
                    m_d = min(m_d, game_data::data->d_mat[game_data::data->my_bases[i2]][i1]);
                    o_d = min(o_d, game_data::data->d_mat[game_data::data->opp_bases[i2]][i1]);
                }
                if (abs(m_d - o_d) <= 2) {
                    vec.push_back(i1);
                }
            }
        }
        return vec;
    }

    vector<pair<int, int> > gather_sorted_resources(int type) {
        vector<pair<int, int> > v;
        vector<int> v2;
        int     i1, i2, tmp;

        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            if (game_data::data->gp_info[i1].type == type && game_data::data->gp_info[i1].resources > 0) {
                tmp = game_data::data->d_mat[i1][game_data::data->my_bases[0]];
                if (game_data::data->number_of_bases > 1 &&
                    game_data::data->d_mat[i1][game_data::data->my_bases[1]] < tmp) {
                        tmp = game_data::data->d_mat[i1][game_data::data->my_bases[1]];
                    }
                v.push_back({tmp, i1});
            }
        }
        sort(v.begin(), v.end());
        if (type == 1) return v;
        v2 = get_crystal_distrubtion();
        for (i1 = 0; i1 < v2.size(); i1++) {
            v.insert(v.begin(), {-1, v2.at(i1)});
        }
        return v;
    }

    int     *generate_moves(void) {
        refere::game_state *s;
        vector<vector<int> > paths;
        vector<int>          p, tmp_p;
        vector<pair<int, int> > v_r, tmp;
        int     *moves, i1, ant_count, crystal_to_win, tmp_dist, egg_t, it;
        float       egg_ratio;

        s = sim::init_game_state(game_data::data->gp_info);
        crystal_to_win = (game_data::data->total_crystal / 2) - game_data::data->my_crystal;
        moves = new int[game_data::data->number_of_cells];
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            moves[i1] = 0;
        } 
        for (i1 = 0; i1 < game_data::data->number_of_bases; i1++) {
            // moves[game_data::data->my_bases[i1]] = 1;
            s->gp_info[game_data::data->my_bases[i1]].beacon = 1;
        }
        ant_count = 0;
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            ant_count += s->gp_info[i1].my_ants;
        }
        egg_ratio = (float) ((ant_count - game_data::data->initial_ants) / game_data::data->number_of_bases) / game_data::data->total_eggs;
        egg_t = (game_data::data->total_eggs / 2) - ((ant_count - game_data::data->initial_ants) / game_data::data->number_of_bases);
        fprintf(stderr, "%d / %d, egg_t %d\n", (ant_count - game_data::data->initial_ants) / 2, game_data::data->total_eggs, egg_t);
        if (egg_ratio < 0.20)
        {
            v_r = gather_sorted_resources(1);
            for (auto r : v_r) {
                p = find_path(s, r.second);
                for (auto cell : p) {
                    moves[cell] = 1;
                    s->gp_info[cell].beacon = 1;
                }
                ant_count -= 2 * p.size();
                egg_t -= s->gp_info[p.back()].resources;
                if (egg_t <= 0 || ant_count <= 0)
                    break ;
            }
        }
        else
        {
            v_r = gather_sorted_resources(2);
            fprintf(stderr, "crystal_need %d\n", crystal_to_win);
            it = 0;
            for (auto r : v_r) {
                p = find_path(s, r.second);
                for (auto cell : p) {
                    moves[cell] = 1;
                    s->gp_info[cell].beacon = 1;
                }
                crystal_to_win -= s->gp_info[r.second].resources;
                if (crystal_to_win <= 0)
                    break ;
                it++;
            }
            v_r = gather_sorted_resources(1);
            for (auto r : v_r) {
                if (egg_t <= 0)
                    break ;
                p = find_path(s, r.second);
                for (auto cell : p) {
                    moves[cell] = 1;
                    s->gp_info[cell].beacon = 1;
                }
                ant_count -= 2 * p.size();
                egg_t -= s->gp_info[p.back()].resources;
            }
        }
        return simulated_annealing(moves);
    }

}

int main()
{
    game_data::data = new game_data::t_data;
    game_data::data->round = 0;
    game_data::data->last_move = 0x0;
    game_data::data->s_zero = 0;
    game_data::data->initial_ants = 0;
    cin >> game_data::data->number_of_cells; cin.ignore();
    game_data::data->gp = new int *[game_data::data->number_of_cells];
    game_data::data->d_mat = new int *[game_data::data->number_of_cells];
    for (size_t i = 0; i < game_data::data->number_of_cells; i++)
    {
        game_data::data->gp[i] = new int [6];
        game_data::data->d_mat[i] = new int [game_data::data->number_of_cells];
    }
    game_data::data->gp_info = new game_data::cell [game_data::data->number_of_cells];
    for (int i = 0; i < game_data::data->number_of_cells; i++) {
        game_data::data->gp_info[i] = {0, 0, 0, 0, 0};
        cin >> game_data::data->gp_info[i].type >> game_data::data->gp_info[i].resources >> game_data::data->gp[i][0] >> game_data::data->gp[i][1] >> game_data::data->gp[i][2] >> game_data::data->gp[i][3] >> game_data::data->gp[i][4] >> game_data::data->gp[i][5]; cin.ignore();
    }
    cin >> game_data::data->number_of_bases; cin.ignore();
    game_data::data->my_bases = new int[game_data::data->number_of_bases];
    game_data::data->opp_bases = new int[game_data::data->number_of_bases];
    for (int i = 0; i < game_data::data->number_of_bases; i++) {
        cin >> game_data::data->my_bases[i]; cin.ignore();
    }
    for (int i = 0; i < game_data::data->number_of_bases; i++) {
        cin >> game_data::data->opp_bases[i]; cin.ignore();
    }
    while (1) {
        refere::game_state *next_state;
        int    *best_s;
        struct timeval start, end;
        string          result;
        long            start_, end_;
        int             i1, i, i2, *tmp_moves;    

        game_data::data->round++;
        game_data::data->my_ants = 0;
        game_data::data->crystal = 0;
        game_data::data->eggs = 0;
        cin >> game_data::data->my_crystal >> game_data::data->opp_crystal; cin.ignore();
        for (i = 0; i < game_data::data->number_of_cells; i++) {
            cin >> game_data::data->gp_info[i].resources >> game_data::data->gp_info[i].my_ants >> game_data::data->gp_info[i].opp_ants; cin.ignore();
            game_data::data->my_ants += game_data::data->gp_info[i].my_ants;
            game_data::data->eggs += game_data::data->gp_info[i].resources * (game_data::data->gp_info[i].type == 1);
            game_data::data->crystal += game_data::data->gp_info[i].resources * (game_data::data->gp_info[i].type == 2);
        }
        gettimeofday(&start, NULL);
        start_ = (start.tv_sec * 1000) + (start.tv_usec / 1000); // us
        game_data::data->start = start_;
        if (game_data::data->round == 1)
            game_data::pre_calculation();
        srand(start_);


        // best_s = game::simulated_annealing();
        best_s = game::generate_moves();
        for (i1 = 0; i1 < game_data::data->number_of_cells; i1++) {
            if (!best_s[i1])
                continue;
            // fprintf(stderr, "(%d, %p)\n", i1, m + i1);
            result += "BEACON " + to_string(i1) + " " + to_string(best_s[i1]) +";";
        }

        if (result.empty()) {
            cout << "WAIT" << endl;
        } else {
            cout << result << endl;
        }
        gettimeofday(&end, NULL);
        end_ = (end.tv_sec * 1000) + (end.tv_usec / 1000); // us
        fprintf(stderr, "time: %lums\n", end_ - start_);
        fflush(stderr);
    }
}
