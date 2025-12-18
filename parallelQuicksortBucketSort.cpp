#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <barrier>
#include <thread>
#include <ctime>
#include <set>
#include <algorithm>
#include <functional>

using namespace std;
vector<thread*> threads;
size_t NUM_THREADS;
barrier<> *bar; 
vector<mutex*> bucket_lks;
struct timespec startTime, endTime;

void global_init(){
	bar = new barrier(NUM_THREADS);
}
void global_cleanup(){
	delete bar;
}
void global_init_buckets(int num_of_lks){
    bucket_lks.resize(num_of_lks);
    for(int i = 0; i < num_of_lks; i++) {
        bucket_lks[i] = new mutex();
    }
}

void global_cleanup_buckets(){
    for(int i = 0; i < bucket_lks.size(); i++) {
        delete bucket_lks[i];
    }
}

//Went with the lomuto's partitioning method rather than Hoares. Selecting the last element as a pivot 
int lomutoPartition(vector<int>& array_of_file_values, int higher_index, int lower_index) {
    int pivot = array_of_file_values[higher_index];//pivot is lat element
    int index = lower_index - 1; 

    for (int i = lower_index; i <= higher_index -1; i++) {//looping from lower to hiher -1
        if (array_of_file_values[i] < pivot) {//if the current array value is less than the pivot 
            index++;
            swap(array_of_file_values[index], array_of_file_values[i]); //swap the element smaller than the pivot to the left side  

        }
    }
    swap(array_of_file_values[index + 1], array_of_file_values[higher_index]);//placing the pivot in the correct position, 
    return index + 1;
}

//Initiation of Quick Sort alg. This and the lomutoPartition fn are responsible for sorting the array_of_file_values.
int quickSort(vector<int>& array_of_file_values, int higher_index, int lower_index) {
    int pivot;
    if (lower_index < higher_index) {//base case is lower_index>= higher_index
        pivot = lomutoPartition(array_of_file_values, higher_index, lower_index);//get pivot to base recursive calls on
        quickSort(array_of_file_values, pivot - 1, lower_index );//sorting left side, smaller than pivot
        quickSort(array_of_file_values,  higher_index, pivot + 1); //sorting the right side, greater than pivot
    }
    return 0;
}

// void* thread_main(size_t tid, vector<int>& sub_array){
int thread_main(size_t tid, vector<int>& sub_array){

    bar->arrive_and_wait();
	if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&startTime);
	}
	quickSort(sub_array, sub_array.size() - 1, 0);
	bar->arrive_and_wait();

	if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&endTime);
	}
	
	return 0;
}

int creatingSubArrays(vector<vector<int>>& array_of_arrays, vector<int>& array_of_file_values) {
    int size_of_single_elements = array_of_file_values.size();
    int array_of_file_values_index = 0;
    int distribution_of_elements = (array_of_file_values.size() / NUM_THREADS); //size = 24, num_thread = 4, distrib==6; size = 27, num_thread =4, distrib == 6
    for (int i = 0; i < NUM_THREADS; i++) {//partitioning the original array into sub arrays.
        array_of_arrays.push_back({});//create a new sub array and store it into array of arrays
        for (int j = 0; j < distribution_of_elements; j++) {
            array_of_arrays[i].push_back(array_of_file_values[array_of_file_values_index]);
            array_of_file_values_index++;
        }
    }

    if (array_of_file_values.size() % NUM_THREADS == 0) return 0;

    for (int j = 0; j < (array_of_file_values.size() % NUM_THREADS); j++) {
        array_of_arrays[array_of_arrays.size() - 1].push_back(array_of_file_values[array_of_file_values_index]);
        array_of_file_values_index++;

    }

    return 0;
}


//partition into sub arrays may come into an issue if the threads is less than 3. Need to think about that more
int parallelizeQuickSort(vector<int>& array_of_file_values, int higher_index, int lower_index) {
    vector<vector<int>> array_of_arrays;
    creatingSubArrays(array_of_arrays, array_of_file_values);
    global_init();
	
	int ret; 
    size_t i;
	threads.resize(NUM_THREADS);
	for(i=1; i<NUM_THREADS; i++){
		threads[i] = new thread(thread_main,i, ref(array_of_arrays[i]));
	}
	i = 0;
	thread_main(i, ref(array_of_arrays[i])); // master also calls thread_main
	
	// join threads
	for(size_t i=1; i<NUM_THREADS; i++){
		threads[i]->join();
		printf("joined thread %zu\n",i);
		delete threads[i];
	}

	global_cleanup();

    int array_with_lowest;
    int min_value;

//merging the sub arrays
    vector<int> indexes_for_all_arrays(NUM_THREADS, 0);
    for (int k = 0; k < array_of_file_values.size(); k++) {
        min_value = -1;
        array_with_lowest = -1;
        
        for (int i = 0; i < NUM_THREADS; i++) {
            if (indexes_for_all_arrays[i] < array_of_arrays[i].size()) {//haven't fully gone through array
                if (array_with_lowest == -1 ) {//inital conditions
                    min_value = array_of_arrays[i][indexes_for_all_arrays[i]];
                    array_with_lowest = i;
                }else if (array_of_arrays[i][indexes_for_all_arrays[i]] < min_value) {//comparing for min value
                    min_value = array_of_arrays[i][indexes_for_all_arrays[i]];
                    array_with_lowest = i;
  
                }
            }
        }
        //Finally changing the main array to be fully sorted.
        if (array_with_lowest != -1) {
            array_of_file_values[k] = min_value;
            indexes_for_all_arrays[array_with_lowest] = indexes_for_all_arrays[array_with_lowest] + 1;
        }
    }
    
	unsigned long long elapsed_ns;
	elapsed_ns = (endTime.tv_sec-startTime.tv_sec)*1000000000 + (endTime.tv_nsec-startTime.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);

    return 0;
}

//Threads will only go after their assigned indexes. This means no skew but if a thread does finish faster, then it won't be moving to get other indexes.
void* fillBuckets(size_t tid, vector<int>& array_of_file_values, vector<set<int>>& buckets, int first_index_for_thread, int last_index_for_thread, int amount_of_buckets, int highest_number, int lowest_number) {
    if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&startTime);
	}

    int bucket_index;
    int threshold;
    threshold = (highest_number - lowest_number + 1) / amount_of_buckets; 
    for (int element_index = first_index_for_thread; element_index < last_index_for_thread; element_index++) {
        bucket_index = (array_of_file_values[element_index] - lowest_number) / threshold; //determining bucket index is still important b/c a thread might be responsible for elements that are in different buckets
        if (bucket_index >= amount_of_buckets) bucket_index = amount_of_buckets - 1; //calculating the bucket_index like this leaves it prone to being the size sometimes.
        bucket_lks[bucket_index]->lock(); //lock the bucket we need to go into to.
        buckets[bucket_index].insert(array_of_file_values[element_index]);
        bucket_lks[bucket_index]->unlock();


    }
    if(tid==0){
		clock_gettime(CLOCK_MONOTONIC,&endTime);
	}

    return 0;
}  

int bucketSort(vector<int>& array_of_file_values, int amount_of_buckets) {
    vector<set<int>> buckets(amount_of_buckets);
    int highest_number = *max_element(array_of_file_values.begin(), array_of_file_values.end());
    int lowest_number = *min_element(array_of_file_values.begin(), array_of_file_values.end());
    int bucket_index;
    int total_elements_per_thread;
    int first_index_for_thread; 
    int last_index_for_thread;
    int array_of_file_values_index = 0;
    global_init_buckets(amount_of_buckets);

    total_elements_per_thread = array_of_file_values.size() / NUM_THREADS;
    size_t i;
	threads.resize(NUM_THREADS);
    for(i=1; i < NUM_THREADS; i++){     
        first_index_for_thread = i * total_elements_per_thread;
        last_index_for_thread = (i + 1) * total_elements_per_thread;
		threads[i] = new thread(fillBuckets,i, ref(array_of_file_values), ref(buckets), first_index_for_thread, last_index_for_thread, amount_of_buckets, highest_number, lowest_number);
	}
	i = 0;
    first_index_for_thread = 0;
    last_index_for_thread = total_elements_per_thread;
    fillBuckets(i, ref(array_of_file_values), ref(buckets), first_index_for_thread, last_index_for_thread, amount_of_buckets, highest_number, lowest_number);
	
	// join threads
	for(size_t i=1; i<NUM_THREADS; i++){
		threads[i]->join();
		printf("joined thread %zu\n",i);
		delete threads[i];
	}
	
	global_cleanup_buckets();
    for(int j=0;j < buckets.size(); j++){
        for(int item: buckets[j]){
            array_of_file_values[array_of_file_values_index] = item;
            array_of_file_values_index++;
        }

	}

    unsigned long long elapsed_ns;
	elapsed_ns = (endTime.tv_sec-startTime.tv_sec)*1000000000 + (endTime.tv_nsec-startTime.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);
    return 0;
}

//Opening/Reading input file and storing the values into an array.
int populateArray(vector<int>& array_of_file_values, string file_name) {
    ifstream intput_file(file_name);
    string line;
    int value;

    if (!intput_file.is_open()){//Error Checking file opening (if errored, else executes)
        printf("There is something wrong with opening your file. Please make sure the file you inputted exists.\n");
        return 1;
    }
    while (getline(intput_file, line)){//Get Each Line
        try {//Try is for the stoi. In case the user's file has an invalid value and it can't convert to a int.
            value = stoi(line);
            array_of_file_values.push_back(value);//stornig file values in the array.
        } catch (const invalid_argument&) {
            printf("The file you inputted has an invalid value. Please modify the file.\n");
            return 1;
        } 
    }
    intput_file.close();
    return 0;
}

//Writing to the output file
int writeToFile(vector<int>& array_of_file_values, string file_name) {
    ofstream output_file(file_name);

    if(output_file.is_open()) {//Error checking for creating the file (if errored, else executes)
        for (int i = 0; i < array_of_file_values.size(); i++) {//for loop to write the values to the output file
            output_file << array_of_file_values[i] << "\n";
        }
        output_file.close();
    } else {
        printf("Can't open the output file.\n");
        return 1;
    }

    return 0;
}

//checks if --name exists and prints my name. Different from other verify fns in that I don't return 1 if there is no --name since the readme doesn't specify that.
int printName(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {//search through arguments
        if (strcmp(argv[i], "--name") == 0){//if --name exists, print the name
            printf("Bazz Khurshid\n");
            break;
        }
    }
    return 0;
}

int verifyThreadsArg(int argc, char* argv[]) {
    int value;
    for (int i = 0; i < argc; ++i) {//search through arguments
        if (strcmp(argv[i], "-t") == 0){
            if (i != (argc -1)) {                
                try {//Try is for the stoi. In case the user's argument after -t is not an integer.
                    value = stoi(argv[i+1]);
                    NUM_THREADS = stoi(argv[i+1]);
                    if (NUM_THREADS < 1) NUM_THREADS = 1;
                    return 0;
                } catch (const invalid_argument&) {
                    printf("The thread argument, NUMTHREADS, in -t [NUMOFTHREADS] must be an integer.\n");
                    return 1;
                } 
            }
        }
    }
    printf("You are missing the argument, -t, for threads, -t [NUMTHREADS].\n");
    return 1;
}

//Every verify_ fn below is about verifying there specific arugmnet. So verify_alg is about verifying the --alg argument. The rest follow after their fn name
int verifyAlgArg(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {//search through arguments
        if (strcmp(argv[i], "--alg=forkjoin") == 0){ //if the alg --alg=merge and --alg=quick
            return 0;
        }else if (strcmp(argv[i], "--alg=lkbucket") == 0){
            return 0;
        }
    }
    printf("You are missing the argument for the algorithm. You can either put --alg=forkjoin or --alg=lkbucket\n");
    return 1;
}

int verifyInputFileArg(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {//search through arguments
        if (strcmp(argv[i], "-i") == 0){
            if (i != (argc -1)) {                
                return 0;
            }
        }
    }
    printf("You are missing the argument for the input file, -i [filename].\n");
    return 1;
}

int verifyOutputFileArg(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {//search through arguments
        if (strcmp(argv[i], "-o") == 0){
            if (i != (argc -1)) {                
                return 0;
            }
        }
    }
    printf("You are missing the argument for the output file, -o [filename].\n");
    return 1;
}

//Verify all the arguments except --name. If they don't exist, then return 1 to signify an error. 
//printName is called but there is no verification
int verifyArguments(int argc, char* argv[]) {
    if (verifyInputFileArg(argc, argv) == 1) return 1;
    if (verifyOutputFileArg(argc, argv) == 1) return 1;
    if (verifyAlgArg(argc, argv) == 1) return 1;
    if (verifyThreadsArg(argc,argv) == 1) return 1;
    printName(argc, argv);
    return 0;
}

int printArray(vector<int>& array_of_file_values) {
    for(int i = 0; i < array_of_file_values.size(); i++) {
        printf("i: %d, array_of_file_values: %d\n", i, array_of_file_values[i]);
    }
    return 0;
}
int main(int argc, char* argv[]) {
    //For when the user just calls the program with --name and nothing else.
    if (argc == 2) {
        if (strcmp(argv[1], "--name") == 0){
            printf("Bazz Khurshid\n");
            return 0;
        }
    }
    //If the user does not have -i, -o or --alg, then I return 1. Crucially, if the call has those but doesn't put a input or output file name, 
    //then I error out on not being able to open the file. For example ./parallelQuicksortBucketSort -i -o outputFile.txt --alg=merge will pass verify_arguments but
    //when it goes to populateArray, the filename being opened, to read, will be -o b/c it is the one after -i. This will then error b/c there should be no
    //file named -o in the user directory. Since this would clearly be a mistake on the user part. 
    if (verifyArguments(argc, argv) == 1) return 1;
    vector<int> array_of_file_values;

    //Through the rest of the main fn, I search through the args each time to find the exact argument.

    //populateArray fn is when I read the input file and store all of its values in the array_of_file_values.
    for (int i = 0; i < argc; ++i) {//Search through the args
        if (strcmp(argv[i], "-i") == 0){
            if (populateArray(array_of_file_values, argv[i+1]) != 0) return 1;
            break;
        }
    }

    for (int i = 0; i < argc; ++i) {//Search through the args
        if (strcmp(argv[i], "--alg=lkbucket") == 0){//if merge, call mergeSort
            bucketSort(array_of_file_values, 10);
            break;
        }else if (strcmp(argv[i], "--alg=forkjoin") == 0){//if quick, call quickSort
            parallelizeQuickSort(array_of_file_values, array_of_file_values.size() - 1, 0);
            break;
        }
    }

    for (int i = 0; i < argc; ++i) {//Search through the args
        if (strcmp(argv[i], "-o") == 0){
            writeToFile(array_of_file_values, argv[i + 1]);//Write the array values into the File, argument after -o (argv[i+1]).  
            break;
        }
    }

    return 0;
}
