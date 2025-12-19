# ParallelQuicksortBucketSort
A fork/join parallel QuickSort and a parallel BucketSort implemetation that sorts a text file of unique integers and outputs the sorted list. In effect, emulating the performance of the UNIX sort -n command.


# Execution Examples
./parallelQuicksortBucketSort -i inputFile.txt -o outputFile.txt -t2 --alg=forkjoin

./parallelQuicksortBucketSort -i inputFile.txt -o outputFile.txt -t4 --alg=lkbucket
