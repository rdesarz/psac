#include <mpi.h>

#include <vector>
#include <iostream>
#include "mpi_samplesort.hpp"
#include "timer.hpp"

typedef int element_t;

class RandInput
{
private:
    int mod;
public:
    RandInput(int seed = 0, int mod = 100) : mod(mod)
    {
        std::srand(seed);
    }

    element_t operator()()
    {
        return 1.13 * (std::rand() % mod);
    }
};

void time_samplesort(std::size_t input_size, MPI_Comm comm)
{
    int p, rank;
    MPI_Comm_size(comm, &p);
    MPI_Comm_rank(comm, &rank);
    timer t;

    // generate local input
    std::size_t local_size = block_partition_local_size(input_size, p, rank);
    std::vector<element_t> local_els(local_size);
    RandInput rand_input(rank, 1000000);
    std::generate(local_els.begin(), local_els.end(), rand_input);
    // sort
    MPI_Barrier(comm);
    double start = t.get_ms();
    samplesort(local_els.begin(), local_els.end(), std::less<element_t>(), comm);
    MPI_Barrier(comm);
    double duration = t.get_ms() - start;
    if (rank == 0)
        // print time taken in csv format
        std::cout << p << ";" << input_size << ";" << duration << std::endl;
    // check if input is sorted
    if (!is_sorted(local_els.begin(), local_els.end(), std::less<element_t>(), comm))
    {
        std::cerr << "ERROR: Output is not sorted!" << std::endl;
        exit(1);
    }
}

void print_usage()
{
    std::cerr << "Usage:  ./test_sort -n <input_size>" << std::endl;
    exit(1);
}

int main(int argc, char *argv[])
{
    // set up MPI
    MPI_Init(&argc, &argv);

    // get communicator size and my rank
    MPI_Comm comm = MPI_COMM_WORLD;
    int p, rank;
    MPI_Comm_size(comm, &p);
    MPI_Comm_rank(comm, &rank);

    // parse input
    std::size_t input_size = 0;
    if (argc < 3)
        print_usage();
    if (argv[1][0] != '-')
        print_usage();

    switch(argv[1][1])
    {
        case 'n':
            // set input_size
            input_size = atol(argv[2]);
            break;
        case 'm':
            // set input_size
            input_size = static_cast<std::size_t>(p*atol(argv[2]));
            break;
        default:
            print_usage();
    }

    if (input_size == 0)
        print_usage();

    // run algorithm
    MPI_Barrier(comm);
    time_samplesort(input_size, comm);

    // finalize MPI
    MPI_Finalize();
    return 0;
}