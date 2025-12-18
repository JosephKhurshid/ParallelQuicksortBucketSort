all: parallelQuicksortBucketSort 

parallelQuicksortBucketSort: parallelQuicksortBucketSort.cpp
	g++ parallelQuicksortBucketSort.cpp -pthread -O3 -std=c++20 -g -o parallelQuicksortBucketSort

clean:
	rm parallelQuicksortBucketSort
