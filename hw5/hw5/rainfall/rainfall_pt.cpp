#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <math.h>

using namespace std;

/* Properties */
// input
int P;      // number of threads
int M;      // total rain drops
double A;   // absorption rate
int N;      // landscape dimension, NxN

// data
vector<vector<vector<pair<int, int>>>> flow_map;
vector<vector<double>> water_level;
vector<vector<double>> absorbed;
vector<vector<int>> grid_height;
vector<vector<double>> level_change;
bool done = false;

// parallel
int num_tasks;
vector<vector<pthread_mutex_t>> lockes;
pthread_barrier_t barrier_update_water;

// result
int step_taken = 0;
double runtime = 0;


/* Helper function */
double calc_time(struct timeval start, struct timeval end) {
    double start_sec = (double)start.tv_sec*1000000.0 + (double)start.tv_usec;
    double end_sec = (double)end.tv_sec*1000000.0 + (double)end.tv_usec;

    if (end_sec < start_sec) {
        return 0;
    } else {
        return end_sec - start_sec;
    }
}

void print_water(){
    cout << "###################" << endl;
    for (int i=0; i<N; ++i){
        for (int j=0; j<N; ++j){
            cout << water_level[i][j] << " ";
        }
        cout << endl;
    }
}

void init_data(string file_name) {
    flow_map = vector<vector<vector<pair<int, int>>>>(N, vector<vector<pair<int, int>>>(N));
    water_level = vector<vector<double>>(N, vector<double>(N, 0.0));
    absorbed = vector<vector<double>>(N, vector<double>(N, 0.0));
    std::ifstream myfile (file_name);
    if (myfile.is_open()){
        for (int i=0; i<N; ++i){
            vector<int> line;
            int h;
            for (int j=0; j<N; ++j){
                myfile >> h;
                line.push_back(h);
            }
            grid_height.push_back(line);
        }
    }
}

void compute_flowmap() {
    // precompute the flow pattern that will occur at each spot
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
}

void print_results(struct timeval start, struct timeval end) {
    runtime = calc_time(start, end);
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
}


/* Main compute function */
void run_one_timestep_point(int i, int j) {
    // falls and absorbs won't affected by other spot in one step.
    // rain falls
    // cout << "run point: " << i << ", " << j << endl;
    if (M > 0){
        water_level[i][j] += 1;
    }
    // absorbed by current spot
    if (water_level[i][j] >= A) {
        water_level[i][j] -= A;
        absorbed[i][j] += A;
    }
    else if (water_level[i][j] > 0) {
        absorbed[i][j] += water_level[i][j];
        water_level[i][j] = 0;
    }
    // flow to other spot
    if (!flow_map[i][j].empty()) {
        double split_cnt = flow_map[i][j].size();
        if (water_level[i][j] >= 1){
            water_level[i][j] -= 1;
            for (auto dest: flow_map[i][j]) {
                // pthread_mutex_lock(&lockes[dest.first][dest.second]);
                level_change[dest.first][dest.second] += 1.0/split_cnt;
                // pthread_mutex_unlock(&lockes[dest.first][dest.second]);
            }
        }
        else if (water_level[i][j] > 0){
            for (auto dest: flow_map[i][j]) {
                // pthread_mutex_lock(&lockes[dest.first][dest.second]);
                level_change[dest.first][dest.second] += water_level[i][j]/split_cnt;
                // pthread_mutex_unlock(&lockes[dest.first][dest.second]);
            }
            water_level[i][j] = 0;
        }
    }
    // not all done once water remain in any point
    if (water_level[i][j] > 0) {
        done = false;
    }
}

void* run_part_simulation(void* args) {
    /* divide landscape by row to parallel groups */
    int thrdID = *(int*)args;
    cout << "run part" << endl;
    cout << "thrdID " << thrdID << ", " << thrdID * num_tasks << endl;

    while (!done) {
        done = true;    // assume true first
        level_change = vector<vector<double>>(N, vector<double>(N, 0));
        for (int i = thrdID * num_tasks; i < min((thrdID + 1)* num_tasks, N); ++i){
            for (int j = 0; j < N; ++j) {                
                run_one_timestep_point(i, j);
            }
        }
        // below check should wait until one whole timestep complete
        // pthread_barrier_wait(&barrier_update_water);
        for (int i = thrdID * num_tasks; i < min((thrdID + 1)* num_tasks, N); ++i){
            for (int j = 0; j < N; ++j) {
                water_level[i][j] += level_change[i][j];    // add water from other point
            }
        }

        step_taken++;
        M--;
    }
}

int main(int argc, char * argv[]){
    if (argc != 6){
        fprintf(stderr, "Argument Count.\n");
    }
    /* read in the parameters */
    P = atoi(argv[1]);      // number of threads
    M = atoi(argv[2]);      // total rain drops
    A = stod(argv[3]);      // absorption rate
    N = atoi(argv[4]);      // landscape dimension, NxN
    std::string file_name = argv[5];
    init_data(file_name);
    num_tasks = ceil(float(N) / P);
    cout << "num tasks " << num_tasks << endl;
    // init mutexes
    for (size_t i = 0; i < N; i++) {
        vector<pthread_mutex_t> line;
        for (size_t j = 0; j < N; j++) {
            line.push_back(PTHREAD_MUTEX_INITIALIZER);
        }
        lockes.push_back(line);
    }
    // init barrier
    pthread_barrier_init(&barrier_update_water, NULL, P);

    /* parallelly run simulation */
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    compute_flowmap();

    // Create the threads
    pthread_t* threads;
    threads = (pthread_t *) malloc(P * sizeof(pthread_t));
    for (int i = 0; i < P; i++) {
        // pass in i as the threadID
        int* thrdID = (int *) malloc(sizeof(int));
        *thrdID = i;
        cout << "thrdID " << i << endl;
        pthread_create(&threads[i], NULL, run_part_simulation, (void *)thrdID);
    }

    for (int i = 0; i < P; i++) {
        pthread_join(threads[i],NULL);
    }

    gettimeofday(&end_time, NULL);

    print_results(start_time, end_time);
    return -1;
}


//  main.cpp
//  ece565hw5

//  Created by mac on 11/15/20.
//  Copyright © 2020 mac. All rights reserved.


// #include <iostream>
// #include <stdlib.h>
// #include <string>
// #include <cstring>
// #include <vector>
// #include <fstream>
// #include <sstream>
// #include <utility>
// #include <float.h>
// #include <cmath>
// #include <algorithm>
// #include <ctime>
// #include <iomanip>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <condition_variable>
// #include <pthread.h>
// #include <omp.h>

// using namespace std;

// pthread_mutex_t** mtxes;
// pthread_mutex_t globalFinishedMtx = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t printMtx = PTHREAD_MUTEX_INITIALIZER;
// pthread_barrier_t barrier1;
// pthread_barrier_t barrier2;
// pthread_barrier_t barrier3;
// pthread_barrier_t barrier4;

// bool globalFinished = true;
// atomic<int> roundNum(0);
// int cnt = 0;

// int threadNum;
// int timeOfRain;
// char * pEnd;
// float absorbRate;
// int N;

// vector<vector<int>> elevations;
// vector<vector<vector<pair<int, int>>>> lowests;
// vector<vector<float>> absorbed;    // calculate how many rains has been absorbed to each cell
// vector<vector<float>> states;      // represent state of each cell at certain time stamp
// vector<vector<float>> trickled;    // calculated how many rains are trickled to this cell at thie time stamp


// void initElevations(const char * filePath) {
//     ifstream infile(filePath);
//     string line;
//     while(getline(infile, line)) {
//         vector<int> vec;
//         stringstream ss(line);
//         string s = "";
//         while(ss >> s) {
//             vec.push_back(stoi(s));
//         }
//         elevations.push_back(vec);
//     }
// }

// // each cell of lowests stores coordinates of neighbors that have same lowest elevation
// void initLowests() {
//     int dr[4] = {-1, 1, 0, 0};
//     int dc[4] = {0, 0, -1, 1};
//     for(int r = 0; r < N; ++r) {
//         for(int c = 0; c < N; ++c) {
//             vector<pair<int, pair<int, int>>> vec;
//             for(int k = 0; k < 4; ++k) {
//                 int nextR = r + dr[k];
//                 int nextC = c + dc[k];
//                 if(nextR >= 0 && nextR < N && nextC >= 0 && nextC < N) {
//                     vec.push_back(make_pair(elevations[nextR][nextC], make_pair(nextR, nextC)));
//                 }
//             }
//             sort(vec.begin(), vec.end(), [](const pair<int, pair<int, int>> p1, const pair<int, pair<int, int>> p2) -> bool {
//                 return p1.first < p2.first;
//             });
//             if(vec[0].first >= elevations[r][c]) {continue;}    // current cell's elevation is the lowest among its neighbors
//             int minVal = vec[0].first;
//             for(size_t m = 0; m < vec.size(); ++m) {
//                 if(vec[m].first > minVal) {break;}
//                 lowests[r][c].push_back(make_pair(vec[m].second.first, vec[m].second.second));
//             }
//         }
//     }
// }

// bool isZero(vector<vector<float>> & states, int N) {
//     for(int i = 0; i < N; ++i) {
//         for(int j = 0; j < N; ++j) {
//             if(abs(states[i][j]) > FLT_EPSILON) {return false;}
//         }
//     }
//     return true;
// }

// void * processSimulation(void* val) {
//     int id = *((int*) val);
//     int threadN = N / threadNum;    // threadN is the number of rows that one thread needs to deal with
//     // Receive a new raindrop (if it is still raining) for each point
//     // If there are raindrops on a point, absorb water into the point
//     // Calculate the number of raindrops that will next trickle to the lowest neighbor(s)
//     int timeStep = 1;
//     while(true) {
//         // cout << "thread " << id << " timeStep: " << timeStep << endl;

//         bool isFinished = true;

//         for(int i = id * threadN; i < (id+1) * threadN; ++i) {
//             for(int j = 0; j < N; ++j) {
//                 if(timeStep <= timeOfRain) {
//                     states[i][j] += 1.0;
//                 }
//                 if(states[i][j] > 0.0) {
//                     float absorbedVal = states[i][j] > absorbRate ? absorbRate : states[i][j];
//                     states[i][j] -= absorbedVal;
//                     absorbed[i][j] += absorbedVal;
//                 }
//                 if(lowests[i][j].size() > 0 && states[i][j] > 0.0) {
//                     float trickledValTotal = states[i][j] >= 1.0 ? 1.0 : states[i][j];
//                     states[i][j] -= trickledValTotal;
                    
//                     float trickledVal = trickledValTotal / lowests[i][j].size();
                    
//                     for(size_t k = 0; k < lowests[i][j].size(); ++k) {
//                         int neiR = lowests[i][j][k].first;
//                         int neiC = lowests[i][j][k].second;
//                         pthread_mutex_lock(&mtxes[neiR][neiC]);
//                         trickled[neiR][neiC] += trickledVal;     // need to be locked
//                         pthread_mutex_unlock(&mtxes[neiR][neiC]);
//                     }
//                 }
//             }
//         }

//         // barrier needed
//         // cout << "thread " << id << " before barrier 1" << endl;
//         pthread_barrier_wait(&barrier1);
//         // cout << "thread " << id << " after barrier 1" << endl;

//         // for test
//         // pthread_mutex_lock(&printMtx);
//         // cout << "thread " << id << " states matrix: " << endl;
//         // for(int i = id * threadN; i < (id+1) * threadN; ++i) {
//         //     for(int j = 0; j < N; ++j) {
//         //         cout << setw(8) << setprecision(6) << states[i][j];
//         //     }
//         //     cout << endl;
//         // }
//         // cout << endl;
//         // pthread_mutex_unlock(&printMtx);
        
//         // update the number of raindrops at each lowest neighbor
//         for(int i = id * threadN; i < (id+1) * threadN; ++i) {
//             for(int j = 0; j < N; ++j) {
//                 states[i][j] += trickled[i][j];
//                 trickled[i][j] = 0.0;
//                 if(abs(states[i][j]) > FLT_EPSILON) {
//                     isFinished = false;
//                     // cout << "inside isFinished false" << endl;
//                 }
//             }
//         }

//         pthread_mutex_lock(&globalFinishedMtx);
//         // cout << "thread " << id << " globalFinished && isFinished:" << endl;
//         // cout << "globalFinished: " << globalFinished << ", " << "isFinished" << isFinished << endl;
//         globalFinished = globalFinished && isFinished;
//         pthread_mutex_unlock(&globalFinishedMtx);

//         // cout << "thread " << id << " before barrier 2" << endl;
//         pthread_barrier_wait(&barrier2);
//         // cout << "thread " << id << " after barrier 2" << endl;

//         if(globalFinished) {
//             roundNum.store(timeStep, memory_order_relaxed);
//             // cout << "thread " << id << " inside globalFinished TRUE" << endl;
//             // cout << "thread " << id << " inside globalFinished TRUE timeStep: " << timeStep << endl;
//             break;
//         } 
//         // cout << "thread " << id << " before barrier 3" << endl;
//         pthread_barrier_wait(&barrier3);
//         // cout << "thread " << id << " after barrier 3" << endl;

//         pthread_mutex_lock(&globalFinishedMtx);
//         globalFinished = true;
//         pthread_mutex_unlock(&globalFinishedMtx);
//         timeStep++;

//         pthread_barrier_wait(&barrier4);
//     }
// }

// int main(int argc, const char * argv[]) {
//     if(argc != 6) {
//         cout << "WRONG INPUT! Should be ./rainfall <P> <M> <A> <N> <elevation_file>" << endl;
//     }
    
//     // initialization & fetch arguments
//     threadNum = atoi(argv[1]);
//     timeOfRain = atoi(argv[2]);
//     absorbRate = strtof(argv[3], &pEnd);
//     N = atoi(argv[4]);
//     pthread_barrier_init (&barrier1, NULL, threadNum);
//     pthread_barrier_init (&barrier2, NULL, threadNum);
//     pthread_barrier_init (&barrier3, NULL, threadNum);
//     pthread_barrier_init (&barrier4, NULL, threadNum);

//     lowests = vector<vector<vector<pair<int, int>>>>(N, vector<vector<pair<int, int>>>(N, vector<pair<int, int>>()));
//     absorbed = vector<vector<float>>(N, vector<float>(N, 0.0));    // calculate how many rains has been absorbed to each cell
//     states = vector<vector<float>>(N, vector<float>(N, 0.0));      // represent state of each cell at certain time stamp
//     trickled = vector<vector<float>>(N, vector<float>(N, 0.0));    // calculated how many rains are trickled to this cell at thie time stamp


//     initElevations(argv[5]);
//     initLowests();

//     // for test
//     // cout << "elevations: " << endl;
//     // for(int i = 0; i < N; ++i) {
//     //     for(int j = 0; j < N; ++j) {
//     //         cout << setw(8) << setprecision(6) << elevations[i][j];
//     //     }
//     //     cout << endl;
//     // }
//     // cout << endl;


//     mtxes = new pthread_mutex_t*[N];
//     for(int i = 0; i < N; ++i) {
//         mtxes[i] = new pthread_mutex_t[N];
//         for(int j = 0; j < N; ++j) {
//             mtxes[i][j] = PTHREAD_MUTEX_INITIALIZER;
//         }
//     }
    
//     // process the simulation
//     struct timeval start_time, end_time;
//     gettimeofday(&start_time, NULL);

//     pthread_t** threads = new pthread_t*[threadNum];
//     int* args = new int[threadNum];
//     for(int i = 0; i < threadNum; ++i) {
//         args[i] = i;
//         threads[i] = new pthread_t;
//         pthread_create(threads[i], NULL, processSimulation, (void*)&(args[i])); 
//     }

//     // cout << "before join" << endl;
//     for(int i = 0; i < threadNum; ++i) {
//         pthread_join(*threads[i], NULL);
//     }
//     // cout << "after join" << endl;
    
//     gettimeofday(&end_time, NULL);
//     double timeUsed = calc_time(start_time, end_time);
    
//     cout << "Rainfall simulation completed in " << roundNum.load(memory_order_relaxed) << " time steps" << endl;
//     cout << "Runtime = " << timeUsed << " seconds" << endl;
//     cout << "The following grid shows the number of raindrops absorbed at each point: \n" << endl;
    
//     for(int i = 0; i < N; ++i) {
//         for(int j = 0; j < N; ++j) {
//             cout << setw(8) << setprecision(6) << absorbed[i][j];
//         }
//         cout << endl;
//     }

//     for(int i = 0; i < N; ++i) {
//         delete[] mtxes[i];
//     }
//     delete[] mtxes;

//     delete[] args;
//     for(int i = 0; i < threadNum; ++i) {
//         delete threads[i];
//     }
//     delete[] threads;
    
//     return 0;
// }