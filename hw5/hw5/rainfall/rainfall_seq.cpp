#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <vector>
#include <cstring>

using namespace std;

double calc_time(struct timeval start, struct timeval end) {
    double start_sec = (double)start.tv_sec*1000000.0 + (double)start.tv_usec;
    double end_sec = (double)end.tv_sec*1000000.0 + (double)end.tv_usec;

    if (end_sec < start_sec) {
        return 0;
    } else {
        return end_sec - start_sec;
    }
}

void print_water(double water_level[][4], int N){
    cout << "###################" << endl;
    for (int i=0; i<N; ++i){
        for (int j=0; j<N; ++j){
            cout << water_level[i][j] << " ";
        }
        cout << endl;
    }
}

int main(int argc, char * argv[]){
    if (argc != 5){
        fprintf(stderr, "Argument Count.\n");
    }
    // read in the parameters
    int M = atoi(argv[1]);      // total rain drops
    double A = stod(argv[2]);   // absorption rate
    int N = atoi(argv[3]);      // landscape dimension, NxN
    std::string file_name = argv[4];
    std::ifstream myfile (file_name);
    int grid_height[N][N];
    if (myfile.is_open()){
        for (int i=0; i<N; ++i){
            for (int j=0; j<N; ++j){
                myfile >> grid_height[i][j];
            }
        }
    }
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    // precompute the flow pattern that will occur at each spot
    vector<vector<vector<pair<int, int>>>> flow_map(N, vector<vector<pair<int, int>>>(N));
    vector<pair<int, int>> masks{{-1,0},{1,0},{0,1},{0,-1}};
    for (int i=0; i<N; ++i){
        for (int j=0; j<N; ++j){
            for (pair<int, int> mask: masks){
                int i_ = i+mask.first;
                int j_ = j+mask.second;
                if (0<=i_ && i_<N && 0<=j_ && j_<N){
                    if (grid_height[i_][j_] < grid_height[i][j]){
                        if (flow_map[i][j].empty()){
                            flow_map[i][j].push_back({i_,j_});
                        }
                        else {
                            if (grid_height[flow_map[i][j].back().first][flow_map[i][j].back().second] > grid_height[i_][j_]){
                                flow_map[i][j].clear();
                                flow_map[i][j].push_back({i_,j_});
                            }
                            else if (grid_height[flow_map[i][j].back().first][flow_map[i][j].back().second] == grid_height[i_][j_]){
                                flow_map[i][j].push_back({i_,j_});
                            }
                            else {
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    // for (int i=0; i<N; ++i){
    //     for (int j=0; j<N; ++j){
    //         for (auto loc: flow_map[i][j]){
    //             cout << loc.first << "-" << loc.second << " ";
    //         }
    //     }
    // }

    // run the simulation
    int iter = 1;
    int step_taken = 0;
    double runtime = 0;
    bool done = false;
    double water_level[N][N];
    memset(water_level, 0, sizeof(water_level));
    double absorbed[N][N];
    memset(absorbed, 0, sizeof(absorbed));
    while (!done){
        done = true;
        double level_change[N][N];
        memset(level_change, 0, sizeof(level_change));
        // cout << "###################" << endl;
        // for (int i=0; i<N; ++i){
        //     for (int j=0; j<N; ++j){
        //         cout << water_level[i][j] << " ";
        //     }
        //     cout << endl;
        // }
        for (int i=0; i<N; ++i){
            for (int j=0; j<N; ++j){
                // rain falls
                if (M > 0){
                    water_level[i][j] += 1;
                }
                // absorbed by current spot
                if (water_level[i][j] >= A){
                    water_level[i][j] -= A;
                    absorbed[i][j] += A;
                }
                else if (water_level[i][j] > 0){
                    absorbed[i][j] += water_level[i][j];
                    water_level[i][j] = 0;
                }
                // flow to other spot
                if (!flow_map[i][j].empty()){
                    double split_cnt = flow_map[i][j].size();
                    if (water_level[i][j] >= 1){
                        water_level[i][j] -= 1;
                        for (auto dest: flow_map[i][j]){
                            level_change[dest.first][dest.second] += 1.0/split_cnt;
                        }
                    }
                    else if (water_level[i][j]>0){
                        for (auto dest: flow_map[i][j]){
                            level_change[dest.first][dest.second] += water_level[i][j]/split_cnt;
                        }
                        water_level[i][j] = 0;
                    }
                }
                // determine if all is done
                if (!done || water_level[i][j]>0){
                    done = false;
                }
            }
        }
        for (int i=0; i<N; ++i){
            for (int j=0; j<N; ++j){

                water_level[i][j] += level_change[i][j];
            }
        }
        step_taken++;
        M--;
    }
    gettimeofday(&end_time, NULL);
    
    // print result
    runtime = calc_time(start_time, end_time);
    cout << "Rainfall simulation took " << step_taken << " time steps to complete." << endl;
    cout << "Runtime = " << runtime/1000000.0 << " seconds." << endl;
    cout << endl;
    cout << "The following grid shows the number of raindrops absorbed at each point:" << endl;
    for (int i=0; i<N; i++){
        for (int j=0; j<N; j++){
            cout.width(8);
            cout << absorbed[i][j];
        }
        cout << endl;
    }
    return -1;
}
