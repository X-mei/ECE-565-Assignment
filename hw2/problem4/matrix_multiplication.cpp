#include <bits/stdc++.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

#define N 1024

double calc_time(struct timeval start, struct timeval end) {
  double start_sec = (double)start.tv_sec*1000000.0 + (double)start.tv_usec;
  double end_sec = (double)end.tv_sec*1000000.0 + (double)end.tv_usec;

  if (end_sec < start_sec) {
    return 0;
  } else {
    return end_sec - start_sec;
  }
}

double get_matrix_mul_time_1(vector<vector<double>>& A, vector<vector<double>>& B, vector<vector<double>> C){
    struct timeval start_time, end_time;
    double sum = 0;
    gettimeofday(&start_time, NULL);
    for (int i=0; i<N; ++i){
        for (int j=0; j<N; ++j){
            sum = 0;
            for (int k=0; k<N; ++k){
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
    gettimeofday(&end_time, NULL);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

double get_matrix_mul_time_2(vector<vector<double>>& A, vector<vector<double>>& B, vector<vector<double>> C){
    struct timeval start_time, end_time;
    double temp;
    gettimeofday(&start_time, NULL);
    for (int j=0; j<N; ++j){
        for (int k=0; k<N; ++k){
            temp = B[k][j];
            for (int i=0; i<N; ++i){
                C[i][j] += temp * A[i][k];
            }
        }
    }
    gettimeofday(&end_time, NULL);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

double get_matrix_mul_time_3(vector<vector<double>>& A, vector<vector<double>>& B, vector<vector<double>> C){
    struct timeval start_time, end_time;
    double temp;
    gettimeofday(&start_time, NULL);
    for (int i=0; i<N; ++i){
        for (int k=0; k<N; ++k){
            temp = A[i][k];
            for (int j=0; j<N; ++j){
                C[i][j] += temp * B[k][j];
            }
        }
    }
    gettimeofday(&end_time, NULL);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

double get_matrix_mul_time_4(vector<vector<double>>& A, vector<vector<double>>& B, vector<vector<double>> C){
    struct timeval start_time, end_time;
    double sum;
    gettimeofday(&start_time, NULL);
    for (int i=0; i<N; i+=32){
        for (int j=0; j<N; j+=32){
            for (int ii=i; ii<i+32; ++ii){
                for (int jj=j; jj<j+32; ++jj){
                    sum = 0;
                    for (int k=0; k<N; ++k){
                        sum += A[ii][k] * B[k][jj];
                    }
                    C[ii][jj] = sum;
                }
            }
        }        
    }
    gettimeofday(&end_time, NULL);
    double res_ns = calc_time(start_time, end_time);
    return res_ns;
}

int main(int argc, char * argv[]){
    if (argc != 2){
        cout << "Usuage: ./matrix_multiplication <option>(1:IJK, 2:JKI, 3:IKJ, 4:Tiling)" << endl;
    }
    vector<vector<double>> A(N, vector<double>(N,1)), B(N, vector<double>(N,2)), C(N, vector<double>(N,0));
    int option = stoi(argv[1]);
    int time_us;
    switch (option){
        case 1:
            time_us = get_matrix_mul_time_1(A, B, C);
            cout << "Runtime with order IJK is: " << time_us/1000000.0 << "sec." << endl;
            break;
        case 2:
            time_us = get_matrix_mul_time_2(A, B, C);
            cout << "Runtime with order JKI is: " << time_us/1000000.0 << "sec." << endl;
            break;
        case 3:
            time_us = get_matrix_mul_time_3(A, B, C);
            cout << "Runtime with order IKJ is: " << time_us/1000000.0 << "sec." << endl;
            break;
        case 4:
            time_us = get_matrix_mul_time_4(A, B, C);
            cout << "Runtime with loop tiling is: " << time_us/1000000.0 << "sec." << endl;
            break;
        default:
            cerr << "Not a valid option." << endl;
            break;
    }
    
    return 0;
}