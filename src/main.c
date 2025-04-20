/**
 * A disk seeking simulation
 *
 * @file main.c
 * @author Joseph Leskey <josleskey@mail.mvnu.edu>
 * @date 1 April 2025
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#define D_SIZE_MIN 0
#define D_SIZE_MAX 65535
#define D_POS_INIT 32767
#define CHUNK true

#define D_DYNAMIC_BASE_SIZE 10

#define safe_malloc(size) _safe_malloc(size, __FILE__, __LINE__)
#define safe_realloc(ptr, size) _safe_realloc(ptr, size, __FILE__, __LINE__)

typedef struct SeekList
{
    int *list;
    int length;
} SeekList;

bool streq(const char *a, const char *b);
int min(const int a, const int b);
int randint(const int min, const int max);

void *_safe_malloc(const size_t size, const char *file, const int line);
void *_safe_realloc(void *ptr, const size_t size, const char *file,
                    const int line);

void printHeader(const char text[]);
void printIntList(const int list[], const int length);

SeekList generateRandomSeeks(const int number);
SeekList extractSeeks(FILE *stream);

void firstComeFirstServed(SeekList *seeks);
void shortestSeekFirst(SeekList *seeks);
void elevatorAlgorithm(SeekList *seeks);

void printOverview(SeekList seeks, bool final);
void printRunStats(SeekList seeks, const char title[]);
void printConclusion();

void processChunk(SeekList chunk);
void processInChunks(SeekList seeks);
void process(SeekList seeks);

int currentStart = -1;
int firstComeStart = D_POS_INIT;
int firstComeTally = 0;
int shortestStart = D_POS_INIT;
int shortestTally = 0;
int elevatorStart = D_POS_INIT;
int elevatorTally = 0;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf(
            "Usage: %s <command>\n"
            "\n"
            "Commands:\n"
            "file <path>    –   read disk seeks from file at path\n"
            "in             –   read disk seeks from stdin\n"
            "rand <number>  –   use given number of random disk seeks\n",
            argv[0]);
    }
    else
    {
        const char *command = argv[1];

        SeekList seeks;

        if (streq(command, "file"))
        {
            if (argc < 3)
            {
                printf("Usage: %s file <path>\n", argv[0]);
                return EXIT_FAILURE;
            }
            else
            {
                const char *path = argv[2];

                FILE *file = fopen(path, "r");

                if (file != NULL)
                {
                    seeks = extractSeeks(file);
                }
                else
                {
                    fprintf(stderr, "Could not open file: %s\n", path);
                    return EXIT_FAILURE;
                }

                fclose(file);
            }
        }
        else if (streq(command, "in"))
        {
            seeks = extractSeeks(stdin);
        }
        else if (streq(command, "rand"))
        {
            if (argc < 3)
            {
                printf("Usage: %s rand <number>\n", argv[0]);
                return EXIT_FAILURE;
            }
            else
            {
                const int number = atoi(argv[2]);
                seeks = generateRandomSeeks(number);
            }
        }
        else
        {
            fprintf(stderr, "Unknown command: %s\n", command);
            return EXIT_FAILURE;
        }

        if (seeks.list != NULL)
        {
            process(seeks);
            free(seeks.list);
        }
        else
        {
            fprintf(stderr, "Failed to create list of disk seeks.\n");
        }
    }
}

bool streq(const char *a, const char *b)
{
    for (; *a != '\0'; a++, b++)
    {
        if (*b == '\0' || *a != *b)
            return false;
    }

    return *b == '\0';
}

SeekList generateRandomSeeks(const int number)
{
    int *seeks = safe_malloc(number * sizeof(int));

    srand(time(NULL));

    for (int i = 0; i < number; i++)
        seeks[i] = randint(D_SIZE_MIN, D_SIZE_MAX);

    return (SeekList){seeks, number};
}

SeekList extractSeeks(FILE *stream)
{
    int size = D_DYNAMIC_BASE_SIZE;
    int *seeks = safe_malloc(sizeof(int) * size);

    int seek;
    int number = 0;

    while (fscanf(stream, "%d\n", &seek) == 1)
    {
        if (number == size)
        {
            size *= 2;
            seeks = safe_realloc(seeks, sizeof(int) * size);
        }

        if (D_SIZE_MIN <= seek && seek <= D_SIZE_MAX)
        {
            seeks[number++] = seek;
        }
        else
        {
            fprintf(stderr, "\nSeek out of bounds: %d\n\n", seek);
        }
    }

    return (SeekList){seeks, number};
}

void process(SeekList seeks)
{
    // Starting position
    const char *initialPositionInput = getenv("D_POS_INIT");
    if (initialPositionInput != NULL)
    {
        int start = atoi(initialPositionInput);
        firstComeStart = start;
        shortestStart = start;
        elevatorStart = start;
    }

#if CHUNK == true
    processInChunks(seeks);
#else
    processChunk(seeks);
#endif

    printOverview(seeks, true);
}

void processInChunks(SeekList seeks)
{
    // I worked out my basic structure before the instructions were
    // updated. I was planning to just process all the requests in one
    // go. Hopefully this addition emulates the sort of table required.
    int buffer[100];
    int remaining = seeks.length;
    int seekIndex = 0;

    for (; seekIndex < min(100, remaining); seekIndex++)
    {
        buffer[seekIndex] = seeks.list[seekIndex];
    }

    while (remaining)
    {
        // Select the next chunk.
        int chunkSize = min(20, remaining);
        SeekList chunk = {buffer, chunkSize};

        // Process chunk.
        processChunk(chunk);

        // Shift buffer content.
        int shiftIndex = chunkSize;
        for (; shiftIndex < 100 && shiftIndex < remaining; shiftIndex++)
        {
            buffer[shiftIndex - chunkSize] = buffer[shiftIndex];
        }

        // Update remaining integer count.
        remaining -= chunkSize;

        // Refill buffer.
        for (int i = shiftIndex - chunkSize; i < 100 && i < remaining; i++)
        {
            buffer[i] = seeks.list[seekIndex++];
        }
    }
}

void processChunk(SeekList seeks)
{
    // Overview
    printOverview(seeks, false);

    // First come, first served algorithm
    firstComeFirstServed(&seeks);
    printRunStats(seeks, "First come, first served");

    // Shortest seek first algorithm
    shortestSeekFirst(&seeks);
    printRunStats(seeks, "Shortest seek first");

    // Elevator algorithm
    elevatorAlgorithm(&seeks);
    printRunStats(seeks, "Elevator algorithm");
}

void printOverview(SeekList seeks, bool final)
{
    printHeader(final ? "Conclusion" : "Overview");

    long sum = 0;

    // Calculate sum.
    for (int i = 0; i < seeks.length; i++)
    {
        sum += seeks.list[i];
    }

    // Calculate mean.
    double mean = sum / (double)seeks.length;

    double sumOfDeviations = 0;

    // Calculate sum of deviations.
    for (int i = 0; i < seeks.length; i++)
    {
        sumOfDeviations += (seeks.list[i] - mean) * (seeks.list[i] - mean);
    }

    // Calculate variance.
    double variance = sumOfDeviations / (double)seeks.length;

    // Calculate standard deviation.
    double stddev = sqrt(variance);

    // Drop the stats.
    printf(
        "Total requested seeks: %d\n"
        "Mean: %.4f\n"
        "Standard deviation: %.4f\n",
        seeks.length, mean, stddev);

    if (final) {
        printConclusion();
    }
}

void printRunStats(SeekList seeks, const char title[])
{
    printHeader(title);

    int distance = 0;
    int seekPosition = currentStart;

    for (int i = 0; i < seeks.length; i++)
    {
        distance += abs(seeks.list[i] - seekPosition);
        seekPosition = seeks.list[i];
    }

    printf("Starting position: %d\n", currentStart);
    printf("Total distance: %d\n", distance);
    printf("\n");
    printIntList(seeks.list, seeks.length);
}

void printConclusion()
{
    printHeader("Effective seek counts");
    printf(
        "First come, first served: %d\n"
        "Shortest seek first: %d\n"
        "Elevator algorithm: %d\n"
        "\n",
        firstComeTally, shortestTally, elevatorTally);
}

void firstComeFirstServed(SeekList *seeks)
{
    currentStart = firstComeStart;

    int lastPosition = firstComeStart;

    for (int i = 0; i < seeks->length; i++)
    {
        if (seeks->list[i] != lastPosition)
        {
            firstComeTally++;
        }
        lastPosition = seeks->list[i];
    }

    firstComeStart = lastPosition;
}

void shortestSeekFirst(SeekList *seeks)
{
    currentStart = shortestStart;

    int seekPosition = shortestStart;

    for (int i = 0; i < seeks->length; i++)
    {
        int bestIndex = -1;
        int smallestDistance = INT_MAX;
        int nextPosition;

        for (int j = i; j < seeks->length; j++)
        {
            int position = seeks->list[j];
            int distance = abs(position - seekPosition);
            if (distance < smallestDistance)
            {
                smallestDistance = distance;
                bestIndex = j;
                nextPosition = position;
            }
        }

        if (bestIndex != -1)
        {
            if (bestIndex != i)
            {
                int currentValue = seeks->list[i];
                seeks->list[i] = seeks->list[bestIndex];
                seeks->list[bestIndex] = currentValue;
            }

            if (seekPosition != nextPosition)
            {
                seekPosition = nextPosition;
                shortestTally++;
            }
        }
    }

    shortestStart = seekPosition;
}

void elevatorAlgorithm(SeekList *seeks)
{
    bool up = true;

    currentStart = elevatorStart;

    int seekPosition = elevatorStart;
    int index = 0;

    for (int run = 2; run > 0; run--, up = !up)
    {
        for (; index < seeks->length; index++)
        {
            int lower = up ? seekPosition : INT_MIN;
            int upper = up ? INT_MAX : seekPosition;

            int position = seeks->list[index];
            int nextIndex = index;

            for (int evalIndex = index; evalIndex < seeks->length; evalIndex++)
            {
                int evalPosition = seeks->list[evalIndex];

                if (lower < evalPosition && evalPosition < upper)
                {
                    seekPosition = evalPosition;
                    nextIndex = evalIndex;

                    if (up)
                    {
                        upper = seekPosition;
                    }
                    else
                    {
                        lower = seekPosition;
                    }
                }
            }

            if (nextIndex != index)
            {
                seeks->list[index] = seekPosition;
                seeks->list[nextIndex] = position;
            }
        }
    }

    seekPosition = elevatorStart;

    for (int i = 0; i < seeks->length; i++)
    {
        if (seeks->list[i] != seekPosition)
        {
            elevatorTally++;
        }
        seekPosition = seeks->list[i];
    }

    elevatorStart = seekPosition;
}

void printHeader(const char text[])
{
    printf("\n%s\n", text);
    for (int i = 0; text[i] != '\0'; i++)
    {
        printf("=");
    }
    printf("\n\n");
}

void printIntList(const int list[], const int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%d%s", list[i], i + 1 == length ? "\n" : ", ");
    }
}

int randint(const int min, const int max)
{
    // Let it be distinctly understood that this is both non-random and
    // potentially non-uniform, but it's probably good enough for this
    // use case.
    return min + rand() % (max - min + 1);
}

int min(const int a, const int b)
{
    return a < b ? a : b;
}

void *_safe_malloc(const size_t size, const char *file, const int line)
{
    void *_ptr = malloc(size);
    if (_ptr == NULL)
    {
        fprintf(stderr, "Allocation error @ %s:%d (%zu bytes)\n", file, line,
                size);
        exit(EXIT_FAILURE);
    }
    return _ptr;
}

void *_safe_realloc(void *ptr, const size_t size, const char *file,
                    const int line)
{
    void *_ptr = realloc(ptr, size);
    if (_ptr == NULL)
    {
        fprintf(stderr, "Allocation error @ %s:%d (%zu bytes)\n", file, line,
                size);
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return _ptr;
}
