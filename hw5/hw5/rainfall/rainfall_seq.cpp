#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <exception>

using namespace std;

class myexception: public exception
{
    virtual const char* what() const throw()
    {
        return "Wrong input parameter count.";
    }
} myex;

double calc_time(struct timeval start, struct timeval end) {
    double start_sec = (double)start.tv_sec*1000000.0 + (double)start.tv_usec;
    double end_sec = (double)end.tv_sec*1000000.0 + (double)end.tv_usec;

    if (end_sec < start_sec) {
        return 0;
    } else {
        return end_sec - start_sec;
    }
}

int main(int argc, char * argv[]){
    /* acquire input parameters */
    int M;                    // total rain drops
    double A;                 // absorption rate
    int N;                    // landscape dimension, NxN
    std::string file_name;
    try {
        if (argc != 5){
            throw myex;
        }
        M = atoi(argv[1]);   
        A = stod(argv[2]);   
        N = atoi(argv[3]);   
        file_name = argv[4];
        
    }
    catch(...){
        fprintf(stderr, "Usage: ./rainfall_seq <steps_with_rain(M)> <absortion_rate(A)> <dimension(N)> <elevation_file>\n");
        abort();
    }
    std::ifstream myfile (file_name);
    vector<vector<int>> grid_height(N, vector<int>(N));    // height at each point
    //int grid_height[N][N];
    if (myfile.is_open()){
        for (int i=0; i<N; ++i){
            for (int j=0; j<N; ++j){
                myfile >> grid_height[i][j];
            }
        }
    }

    /* start of timer */
    struct timeval start_time, end_time; // variable to compute time consumption
    double runtime = 0;                  // runtime of simulation, including flow chart precomputation
    gettimeofday(&start_time, NULL);

    /* precompute the flow pattern that will occur at each spot */
    // the destination of water flow at each point
    vector<vector<vector<pair<int, int>>>> flow_map(N, vector<vector<pair<int, int>>>(N));
    // for easy computation of neighbors coordinate
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

    /* run the rainfall simulation */
    int step_taken = 0;          // time step count
    bool done = false;           // indicator of weather simulation should stop, i.e., no water on each point
    vector<vector<double>> water_level(N, vector<double>(N));    // realtime record of water level on each point
    // double water_level[N][N];
    // memset(water_level, 0, sizeof(water_level));
    vector<vector<double>> absorbed(N, vector<double>(N));    // aggregated amount of water absorbed
    // double absorbed[N][N];
    // memset(absorbed, 0, sizeof(absorbed));
    vector<vector<double>> level_change(N, vector<double>(N));      // record the water flowing from other point to each point at each time step
    while (!done){
        done = true;
        // std::cout << step_taken << endl;
        //std::fill(level_change.begin(), level_change.end(), 0.0);
        //memset(level_change, 0, sizeof(level_change));
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
                // determine if all water disipated
                if (!done || water_level[i][j]>0){
                    done = false;
                }
            }
        }
        for (int i=0; i<N; ++i){
            for (int j=0; j<N; ++j){
                water_level[i][j] += level_change[i][j];
                level_change[i][j] = 0;
            }
        }
        step_taken++;
        M--;
    }
    gettimeofday(&end_time, NULL);
    /* end of timer */

    /* print result */
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
