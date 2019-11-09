#include <cstdlib>
#include <ctime>
#include <random>
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <iomanip>
#include <chrono>
#include <unistd.h>
#include <map>

using namespace std::chrono;
using namespace std;
#define N_THREADS_PER_BLOCK 1024

__device__ double atomicAddDouble(double* address, double val)
{
    unsigned long long int* address_as_ull =
                             (unsigned long long int*)address;
    unsigned long long int old = *address_as_ull, assumed;
    do {
        assumed = old;
        old = atomicCAS(address_as_ull, assumed, __double_as_longlong(val +         __longlong_as_double(assumed)));
    } while (assumed != old);
    return __longlong_as_double(old);
}

class MatrixCUDA{
    public:
        int *dim;
        float *matrix;
        int *dev_dim;
        float *dev_matrix;

        MatrixCUDA(int *n){
            
            matrix = (float* )malloc(sizeof(float)*(*n)*(*n));
            
            cudaMalloc( (void**)&dev_matrix, sizeof(float)*(*n)*(*n));
            for(int i=0; i< (*n)*(*n); i++){
        
        		matrix[i] = 0.0;
   			}

    		cudaMemcpy( dev_matrix, matrix, sizeof(float)*(*n)*(*n), cudaMemcpyHostToDevice );
                  
            dim = (int* )malloc(sizeof(int));
            *dim = *n;
            cudaMalloc((void**)&dev_dim, sizeof(int) );
            cudaMemcpy( dev_dim, dim, sizeof(int), cudaMemcpyHostToDevice);
            
        }

        void random_init(float *lower_limit, float *higher_limit);
        void print_matrix();
        void clear_mem();
};

void MatrixCUDA::clear_mem(){

    /*for(int i=0 ; i<(*dim)*(*dim); i++) {
        cudaFree(dev_matrix[i]);
    
    }*/
    free(dim);
    cudaFree(dev_dim);
    
    free(matrix);
    cudaFree(dev_matrix);

    return;

}

void MatrixCUDA::random_init(float *lower_limit, float *higher_limit){
    //float **random_numbers;
        //random_numbers = (float** )malloc(sizeof(float*)*(*dim));
    
            
    for(int i=0; i< (*dim)*(*dim); i++){
        //random_numbers[i] = (float* )malloc(sizeof(float)*(*dim));
        //for(int j=0; j< *dim; j++){
            //float random_number;
            //random_number =  (float* )malloc(sizeof(float));
            
            //random_number = 
            matrix[i] = *lower_limit + static_cast <float> (rand()) /(static_cast <float> (RAND_MAX/( *higher_limit - *lower_limit)));
            //free(random_number);

        //}

    }

    cudaMemcpy( dev_matrix, matrix, sizeof(float)*(*dim)*(*dim), cudaMemcpyHostToDevice );

    return;
}

void MatrixCUDA::print_matrix(){
    

    cudaMemcpy( matrix, dev_matrix, sizeof(float)*(*dim)*(*dim), cudaMemcpyDeviceToHost );   
   
    for(int i=0; i<*dim; i++){
        for(int j=0; j<*dim; j++){
            cout << matrix[i*(*dim)+j]<< "  ";
        }
        cout << endl;
    }


    cout << "Dimensions is " << *dim << " X " << *dim << endl;

    return;
}

    
__global__ void mult(float *a_matrix, float *b_matrix, float *c_matrix, int *dim){
    
    int n = *dim;
    __shared__ float temp_sum[N_THREADS_PER_BLOCK];

    int z = int((n+N_THREADS_PER_BLOCK-1)/N_THREADS_PER_BLOCK);
    int dim1 = int((blockIdx.x)/(z*n));
    int dim2 = int((blockIdx.x)/z)%n;
    int dim3 = (blockIdx.x)%z;

	int index = dim3*(N_THREADS_PER_BLOCK) + threadIdx.x;
	
	if( index > (*dim)){
    	temp_sum[threadIdx.x] = 0;
    }

    else{
	temp_sum[threadIdx.x] = a_matrix[dim1*(*dim) + index]* b_matrix[(index)*(*dim) + dim2];
	// (*counta)++;
	}
    __syncthreads();

    if( 0 == threadIdx.x){
        float block_sum = 0;
        for (int i = 0; i < (N_THREADS_PER_BLOCK) ; i++){
            block_sum += temp_sum[i];
            // (*countb)++;	
        }
        float new_sum = block_sum;
        atomicAdd(&(c_matrix[dim1*(*dim)+dim2]), new_sum);
    }
    return;
}

int main(){
    //   500 - 51
    //  1000 - 51
    //  2000 - 86
    //  4000 - 223
    //  8000 - 753
    // 10000 - 1165
    // 12000 - 1669
    // 14000 - 2264
    // 16000 - 1991
    // 18000 - 2493
    // 20000 - 1547
    // 22000 - 1867
    // 26000 - 2598

	std::map<int, int> load;
	
	load[50] = 1000;
	load[75] = 2000;
	load[200] = 4000;
	load[750] = 8000;
	load[1100] = 10000;
	load[1600] = 12000;
	load[1850] = 22000;
	load[2000] = 16000;
	load[2200] = 14000;
	load[2500] = 18000;
	load[2600] = 26000;
	int load_wanted;// = 8000;
    cout << "Enter load in MB and press enter" << endl;
    cout << "Select from the following (50, 75, 200, 750, 1100, 1600, 1850, 2000, 2200, 2500 or 2600)" << endl;

    srand (static_cast <unsigned> (5));
    cin >> load_wanted;
    cout << "Loaded" << endl;
    cout << "Press Ctrl C if you love your GPU" << endl;
    int matrix_dim = load[load_wanted];
    while(true){
        MatrixCUDA a(&matrix_dim);
        MatrixCUDA b(&matrix_dim);
        MatrixCUDA c(&matrix_dim);
        float lower_limit;
        float upper_limit;

        lower_limit = -2;
        upper_limit = 2;
        a.random_init(&lower_limit, &upper_limit);

        b.random_init(&lower_limit, &upper_limit);
       

        
        int dimension3 =  ceil((matrix_dim*1.0)/float(N_THREADS_PER_BLOCK));
        int n_blocks = (matrix_dim*matrix_dim*dimension3);
         float dur;
        cudaEvent_t start, stop;

        cudaEventCreate(&start) ;
        cudaEventCreate(&stop) ;
        cudaEventRecord(start, 0) ;

        
        mult <<< n_blocks, N_THREADS_PER_BLOCK, N_THREADS_PER_BLOCK>>>(a.dev_matrix, b.dev_matrix, c.dev_matrix, a.dev_dim);

        cudaEventRecord(stop, 0) ;
        cudaEventSynchronize(stop) ;
        cudaEventElapsedTime(&dur, start, stop);

        a.clear_mem();
        b.clear_mem();
        c.clear_mem();                
    }
    //}
    
    return 0;
}


    // srand (static_cast <unsigned> (5));
    // int matrix_size[4] = {16, 128, 1024,1024*16,};
    // //cout << "Matrix dimension" << "     N_THREADS_PER_BLOCK" << "    time(microseconds)"<< endl;
    // //for (int i =0 ; i<4; i++){
    //     int matrix_dim = 1024;// matrix_size[i];
    //     MatrixCUDA a(&matrix_dim);
    //     MatrixCUDA b(&matrix_dim);
        
    //     MatrixCUDA c(&matrix_dim);
    //     float lower_limit;
    //     float upper_limit;

    //     lower_limit = -1000.0;
    //     upper_limit = 1000.0;
    //     a.random_init(&lower_limit, &upper_limit);
        
    //     //a.print_matrix();
    //     b.random_init(&lower_limit, &upper_limit);
    //     c.random_init(&lower_limit, &upper_limit);

    //     //c.print_matrix();
    //     //int threads[9] = {4,8,16,32,64,128,256,512,1024};

    //     //for(int j = 0; j<9; j++){
    //         int average = 0;
            
    //         const int N_THREADS_PER_BLOCK = 1024;//threads[j];///threads[j];

    //         // if (N_THREADS_PER_BLOCK > matrix_dim){
    //         //     break;
    //         // }  
    //         //for (int k = 0; k<10; k++){
                
    //             //cudaEvent_t start, stop;
    //             //cudaEventCreate(&start);
    //             //cudaEventCreate(&stop);
    //             //cudaEventRecord(start);
    //             //cout<<"here"<<endl;
    //             auto start = high_resolution_clock::now(); 
    //             int n_blocks = ceil((matrix_dim*matrix_dim*matrix_dim*1.0)/float(N_THREADS_PER_BLOCK));
    //             cout<<n_blocks<<" "<<N_THREADS_PER_BLOCK << endl;
    //             mult <<< n_blocks, N_THREADS_PER_BLOCK, N_THREADS_PER_BLOCK>>>(a.dev_matrix, b.dev_matrix, c.dev_matrix, a.dev_dim, N_THREADS_PER_BLOCK);
    //             //cout<<"2"<<endl;
    //             //cudaEventRecord(stop);
    //             //cout << "end"<<endl;
    //             //cudaEventSynchronize(stop);
    //             //float milliseconds = 0;
    //             //cudaEventElapsedTime(&milliseconds, start, stop);
                
    //             auto stop = high_resolution_clock::now();
                
    //             auto duration = duration_cast<nanoseconds>(stop - start); 
    //             //cout << "Matrix dimension is " << matrix_dim << " X "<<  matrix_dim << " and the N_THREADS_PER_BLOCK are " << N_THREADS_PER_BLOCK << "and the time take is " << duration.count() << " microseconds" << endl;//<< milliseconds << endl;//
    //         //     if(k >1){
    //         //         average += duration.count();
    //         //     }
    //         // }

    //         average =average/8;

    //         cout << matrix_dim << " X "<<  matrix_dim << " \t \t " << N_THREADS_PER_BLOCK << " \t \t " << average << endl;
             
    //     //}
    //     // a.print_matrix();
    //     // b.print_matrix();
    //     //c.print_matrix();
    //     a.clear_mem();
    //     b.clear_mem();dim
    //     c.clear_mem();                
    
    // //}
