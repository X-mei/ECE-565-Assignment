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
bool all_done = false;

// parallel
int num_tasks;
vector<vector<pthread_mutex_t>> lockes;
pthread_mutex_t lock_step;
pthread_mutex_t lock_done;
pthread_barrier_t barrier_new_loop;
pthread_barrier_t barrier_update_water;
pthread_barrier_t barrier_all_done;

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
    level_change = vector<vector<double>>(N, vector<double>(N, 0));
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

void print_time(struct timeval start, struct timeval end) {
    runtime = calc_time(start, end);
    cout << "Rainfall simulation took " << step_taken << " time steps to complete." << endl;
    cout << "Runtime = " << runtime/1000000.0 << " seconds." << endl;
    cout << endl;
}

void print_absorb() {
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
    if (M > step_taken){
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
                pthread_mutex_lock(&lockes[dest.first][dest.second]);
                level_change[dest.first][dest.second] += 1.0/split_cnt;
                pthread_mutex_unlock(&lockes[dest.first][dest.second]);
            }
        }
        else if (water_level[i][j] > 0){
            for (auto dest: flow_map[i][j]) {
                pthread_mutex_lock(&lockes[dest.first][dest.second]);
                level_change[dest.first][dest.second] += water_level[i][j]/split_cnt;
                pthread_mutex_unlock(&lockes[dest.first][dest.second]);
            }
            water_level[i][j] = 0;
        }
    }
}

void* run_part_simulation(void* args) {
    /* divide landscape by row to parallel groups */
    int thrdID = *(int*)args;
    // cout << "run part" << endl;
    // cout << "thrdID " << thrdID << ", " << thrdID * num_tasks << endl;

    int step = 0;
    while (!all_done) {
        pthread_barrier_wait(&barrier_new_loop);
        for (int i = thrdID * num_tasks; i < min((thrdID + 1)* num_tasks, N); ++i){
            for (int j = 0; j < N; ++j) {                
                run_one_timestep_point(i, j);
            }
        }
        all_done = true;    // assume true first

        // below check should wait until all parts in one whole timestep complete
        pthread_barrier_wait(&barrier_update_water);
        for (int i = thrdID * num_tasks; i < min((thrdID + 1)* num_tasks, N); ++i){
            for (int j = 0; j < N; ++j) {
                water_level[i][j] += level_change[i][j];    // add water from other point
                level_change[i][j] = 0;     // reset change calculation in each step
                // not all done once water remain in any point
                if (water_level[i][j] > 0) {
                    all_done = false;
                }
            }
        }
        step++;

        // enter new loop only all parts is checked if done
        pthread_barrier_wait(&barrier_all_done);
        step_taken = step;
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
    // cout << "num tasks " << num_tasks << endl;
    // init mutexes
    for (size_t i = 0; i < N; i++) {
        vector<pthread_mutex_t> line;
        for (size_t j = 0; j < N; j++) {
            line.push_back(PTHREAD_MUTEX_INITIALIZER);
        }
        lockes.push_back(line);
    }
    // init barrier
    pthread_barrier_init(&barrier_new_loop, NULL, P);
    pthread_barrier_init(&barrier_update_water, NULL, P);
    pthread_barrier_init(&barrier_all_done, NULL, P);

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
        pthread_create(&threads[i], NULL, run_part_simulation, (void *)thrdID);
    }

    for (int i = 0; i < P; i++) {
        pthread_join(threads[i],NULL);
    }

    gettimeofday(&end_time, NULL);

    print_time(start_time, end_time);
    print_absorb();
    return -1;
}
